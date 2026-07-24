#include "complexity_estimator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Known C standard library function complexities */
static const char *_known_o_n_log_n[] = {
    "qsort", "heapsort", "mergesort", NULL
};
static const char *_known_o_log_n[] = {
    "bsearch", NULL
};

static const char *_known_o_n[] = {
    "strlen", "strcpy", "strcmp", "memcpy", "memset",
    "atoi", "atof", "printf", "fprintf", "sprintf",
    "snprintf", "puts", "gets", "fgets", "fread",
    "fwrite", "strdup", "strndup",
    "strchr", "strrchr", "strstr", "memchr",
    "index", "rindex",
    "calloc", "malloc", "realloc",
    NULL
};
static const char *_known_o_1[] = {
    "sizeof", "free",
    "abs", "labs", "llabs",
    "tolower", "toupper",
    "isalnum", "isalpha", "iscntrl", "isdigit",
    "isgraph", "islower", "isprint", "ispunct",
    "isspace", "isupper", "isxdigit",
    NULL
};

ComplexityClass get_known_function_complexity(const char *func_name)
{
    if (!func_name) return COMPLEXITY_UNKNOWN;

    for (int i = 0; _known_o_n_log_n[i]; i++) {
        if (strcmp(func_name, _known_o_n_log_n[i]) == 0)
            return COMPLEXITY_O_N_LOG_N;
    }
    for (int i = 0; _known_o_log_n[i]; i++) {
        if (strcmp(func_name, _known_o_log_n[i]) == 0)
            return COMPLEXITY_O_LOG_N;
    }
    for (int i = 0; _known_o_n[i]; i++) {
        if (strcmp(func_name, _known_o_n[i]) == 0)
            return COMPLEXITY_O_N;
    }
    for (int i = 0; _known_o_1[i]; i++) {
        if (strcmp(func_name, _known_o_1[i]) == 0)
            return COMPLEXITY_O_1;
    }

    return COMPLEXITY_UNKNOWN;
}

ComplexityResult *complexity_result_create(const char *func_name, int lineno)
{
    ComplexityResult *r = (ComplexityResult *)calloc(1, sizeof(ComplexityResult));
    if (!r) return NULL;
    r->function_name = strdup(func_name);
    r->lineno = lineno;
    r->end_lineno = lineno;
    r->estimated_complexity = COMPLEXITY_O_1;
    r->confidence = 1.0;
    return r;
}

void complexity_result_free(ComplexityResult *result)
{
    if (!result) return;
    free(result->function_name);
    for (size_t i = 0; i < result->explanations_count; i++) {
        free(result->explanations[i].source);
        free(result->explanations[i].description);
        free(result->explanations[i].detail);
    }
    free(result->explanations);
    if (result->loop_analysis) loop_analysis_free(result->loop_analysis);
    if (result->recursion_info) recursion_info_free(result->recursion_info);
    free(result->bottleneck_lines);
    free(result->bottleneck_description);
    for (size_t i = 0; i < result->warnings_count; i++)
        free(result->warnings[i]);
    free(result->warnings);
    free(result);
}

static void _add_explanation(ComplexityResult *r, const char *source,
                              ComplexityClass c, int lineno,
                              const char *desc, const char *detail)
{
    size_t idx = r->explanations_count;
    ComplexityExplanation *new_e = (ComplexityExplanation *)realloc(
        r->explanations, (idx + 1) * sizeof(ComplexityExplanation));
    if (!new_e) return;
    r->explanations = new_e;
    r->explanations_count++;
    r->explanations[idx].source = strdup(source);
    r->explanations[idx].complexity = c;
    r->explanations[idx].lineno = lineno;
    r->explanations[idx].description = strdup(desc);
    r->explanations[idx].detail = detail ? strdup(detail) : strdup("");
}

static void _add_warning(ComplexityResult *r, const char *warning)
{
    char **new_w = (char **)realloc(r->warnings, (r->warnings_count + 1) * sizeof(char *));
    if (!new_w) return;
    r->warnings = new_w;
    r->warnings[r->warnings_count++] = strdup(warning);
}

ComplexityClass combine_complexities(ComplexityClass c1, ComplexityClass c2,
                                      const char *operation)
{
    if (strcmp(operation, "max") == 0) {
        return complexity_class_rank(c1) >= complexity_class_rank(c2) ? c1 : c2;
    }

    /* Multiply — used for nested loops.
     * Use polynomial degree math: O(n) × O(n²) = O(n³) */
    double deg1 = 0, deg2 = 0;
    switch (c1) {
        case COMPLEXITY_O_1: deg1 = 0; break;
        case COMPLEXITY_O_LOG_N: deg1 = 0.5; break;
        case COMPLEXITY_O_N: deg1 = 1; break;
        case COMPLEXITY_O_N_LOG_N: deg1 = 1.5; break;
        case COMPLEXITY_O_N_SQUARED: deg1 = 2; break;
        case COMPLEXITY_O_N_CUBED: deg1 = 3; break;
        case COMPLEXITY_O_N_K: deg1 = 4; break;
        case COMPLEXITY_O_2_N: deg1 = 100; break;
        case COMPLEXITY_O_N_FACTORIAL: deg1 = 200; break;
        default: deg1 = 0; break;
    }
    switch (c2) {
        case COMPLEXITY_O_1: deg2 = 0; break;
        case COMPLEXITY_O_LOG_N: deg2 = 0.5; break;
        case COMPLEXITY_O_N: deg2 = 1; break;
        case COMPLEXITY_O_N_LOG_N: deg2 = 1.5; break;
        case COMPLEXITY_O_N_SQUARED: deg2 = 2; break;
        case COMPLEXITY_O_N_CUBED: deg2 = 3; break;
        case COMPLEXITY_O_N_K: deg2 = 4; break;
        case COMPLEXITY_O_2_N: deg2 = 100; break;
        case COMPLEXITY_O_N_FACTORIAL: deg2 = 200; break;
        default: deg2 = 0; break;
    }

    double sum = deg1 + deg2;
    if (sum <= 0) return COMPLEXITY_O_1;
    if (sum <= 0.5) return COMPLEXITY_O_LOG_N;
    if (sum <= 1) return COMPLEXITY_O_N;
    if (sum <= 1.5) return COMPLEXITY_O_N_LOG_N;
    if (sum <= 2) return COMPLEXITY_O_N_SQUARED;
    if (sum <= 3) return COMPLEXITY_O_N_CUBED;
    if (sum <= 4) return COMPLEXITY_O_N_K;
    if (sum <= 100) return COMPLEXITY_O_2_N;
    return COMPLEXITY_O_N_FACTORIAL;
}

/* Analyze function calls in a body and their known complexities */
static void _analyze_body_calls(CioptNode *node, ComplexityResult *result,
                                 ComplexityClass *call_complexity)
{
    if (!node) return;

    if (node->type == CIOPT_NODE_CALL && node->data.call.name) {
        ComplexityClass known = get_known_function_complexity(node->data.call.name);
        if (known != COMPLEXITY_UNKNOWN) {
            if (complexity_class_rank(known) > complexity_class_rank(*call_complexity))
                *call_complexity = known;

            char desc[256];
            snprintf(desc, sizeof(desc), "Call to '%s' has %s complexity",
                     node->data.call.name, complexity_class_to_string(known));
            _add_explanation(result, "call", known, node->lineno, desc, "");
        }
    }

    /* Recurse into children */
    switch (node->type) {
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                _analyze_body_calls(node->data.block.stmts.nodes[i], result, call_complexity);
            break;
        case CIOPT_NODE_IF:
            _analyze_body_calls(node->data.if_stmt.condition, result, call_complexity);
            _analyze_body_calls(node->data.if_stmt.then_body, result, call_complexity);
            _analyze_body_calls(node->data.if_stmt.else_body, result, call_complexity);
            break;
        case CIOPT_NODE_FOR:
            _analyze_body_calls(node->data.for_loop.init, result, call_complexity);
            _analyze_body_calls(node->data.for_loop.condition, result, call_complexity);
            _analyze_body_calls(node->data.for_loop.update, result, call_complexity);
            _analyze_body_calls(node->data.for_loop.body, result, call_complexity);
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            _analyze_body_calls(node->data.loop.condition, result, call_complexity);
            _analyze_body_calls(node->data.loop.body, result, call_complexity);
            break;
        case CIOPT_NODE_RETURN:
            _analyze_body_calls(node->data.return_stmt.value, result, call_complexity);
            break;
        case CIOPT_NODE_EXPR_STMT:
            _analyze_body_calls(node->data.expr_stmt.expr, result, call_complexity);
            break;
        case CIOPT_NODE_BINARY_OP:
            _analyze_body_calls(node->data.binary_op.left, result, call_complexity);
            _analyze_body_calls(node->data.binary_op.right, result, call_complexity);
            break;
        case CIOPT_NODE_ASSIGNMENT:
            _analyze_body_calls(node->data.assignment.value, result, call_complexity);
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++)
                _analyze_body_calls(node->data.call.args.nodes[i], result, call_complexity);
            break;
        default:
            break;
    }
}

/* Find the maximum *algorithmic* complexity of function calls inside this loop.
 * Only considers O(log n) and O(n log n) functions (search/sort algorithms).
 * O(n) string/IO ops are already handled by has_expensive_operation in loop_detector. */
static ComplexityClass _find_max_known_call_complexity(CioptNode *node)
{
    if (!node) return COMPLEXITY_O_1;

    ComplexityClass max_c = COMPLEXITY_O_1;

    if (node->type == CIOPT_NODE_CALL && node->data.call.name) {
        ComplexityClass known = get_known_function_complexity(node->data.call.name);
        /* Only multiply by algorithmic functions (log n, n log n), not O(n) I/O */
        if (known != COMPLEXITY_UNKNOWN && known != COMPLEXITY_O_N && known != COMPLEXITY_O_1) {
            max_c = known;
        }
    }

    /* Recurse into children except loops */
    switch (node->type) {
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++) {
                ComplexityClass child_c = _find_max_known_call_complexity(node->data.block.stmts.nodes[i]);
                if (complexity_class_compare(child_c, max_c) > 0)
                    max_c = child_c;
            }
            break;
        case CIOPT_NODE_IF: {
            if (node->data.if_stmt.condition) {
                ComplexityClass cond_c = _find_max_known_call_complexity(node->data.if_stmt.condition);
                if (complexity_class_compare(cond_c, max_c) > 0)
                    max_c = cond_c;
            }
            ComplexityClass then_c = _find_max_known_call_complexity(node->data.if_stmt.then_body);
            ComplexityClass else_c = _find_max_known_call_complexity(node->data.if_stmt.else_body);
            if (complexity_class_compare(then_c, max_c) > 0)
                max_c = then_c;
            if (complexity_class_compare(else_c, max_c) > 0)
                max_c = else_c;
            break;
        }
        case CIOPT_NODE_RETURN:
            if (node->data.return_stmt.value) {
                ComplexityClass ret_c = _find_max_known_call_complexity(node->data.return_stmt.value);
                if (complexity_class_compare(ret_c, max_c) > 0)
                    max_c = ret_c;
            }
            break;
        case CIOPT_NODE_EXPR_STMT:
            if (node->data.expr_stmt.expr) {
                ComplexityClass expr_c = _find_max_known_call_complexity(node->data.expr_stmt.expr);
                if (complexity_class_compare(expr_c, max_c) > 0)
                    max_c = expr_c;
            }
            break;
        case CIOPT_NODE_BINARY_OP: {
            ComplexityClass left_c = _find_max_known_call_complexity(node->data.binary_op.left);
            ComplexityClass right_c = _find_max_known_call_complexity(node->data.binary_op.right);
            if (complexity_class_compare(left_c, max_c) > 0)
                max_c = left_c;
            if (complexity_class_compare(right_c, max_c) > 0)
                max_c = right_c;
            break;
        }
        case CIOPT_NODE_ASSIGNMENT:
            if (node->data.assignment.value) {
                ComplexityClass val_c = _find_max_known_call_complexity(node->data.assignment.value);
                if (complexity_class_compare(val_c, max_c) > 0)
                    max_c = val_c;
            }
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++) {
                ComplexityClass arg_c = _find_max_known_call_complexity(node->data.call.args.nodes[i]);
                if (complexity_class_compare(arg_c, max_c) > 0)
                    max_c = arg_c;
            }
            break;
        default:
            break;
    }

    return max_c;
}

static ComplexityClass _calculate_single_loop_complexity(LoopDetail *loop)
{
    if (!loop) return COMPLEXITY_O_1;

    ComplexityClass self_c = estimate_loop_iterations(loop);

    /* Check for expensive O(N) calls inside this loop - makes it O(n^2) */
    if (loop->has_expensive_operation && self_c == COMPLEXITY_O_N) {
        self_c = COMPLEXITY_O_N_SQUARED;
    }

    /* Multiply iteration complexity by the complexity of any known function call inside the loop */
    CioptNode *body = NULL;
    if (loop->node) {
        if (loop->node->type == CIOPT_NODE_WHILE || loop->node->type == CIOPT_NODE_DO_WHILE)
            body = loop->node->data.loop.body;
        else if (loop->node->type == CIOPT_NODE_FOR)
            body = loop->node->data.for_loop.body;
    }
    ComplexityClass max_call_c = _find_max_known_call_complexity(body);
    self_c = combine_complexities(self_c, max_call_c, "multiply");

    if (loop->children_count == 0) return self_c;

    ComplexityClass max_child_c = COMPLEXITY_O_1;
    for (size_t i = 0; i < loop->children_count; i++) {
        ComplexityClass child_c = _calculate_single_loop_complexity(loop->children[i]);
        if (complexity_class_rank(child_c) > complexity_class_rank(max_child_c)) {
            max_child_c = child_c;
        }
    }

    return combine_complexities(self_c, max_child_c, "multiply");
}

/* Estimate complexity contribution from loops */
static ComplexityClass _estimate_loop_complexity(LoopAnalysis *loop_analysis,
                                                   ComplexityResult *result)
{
    if (!loop_analysis || loop_analysis->total_loops == 0)
        return COMPLEXITY_O_1;

    ComplexityClass max_complexity = COMPLEXITY_O_1;

    for (size_t i = 0; i < loop_analysis->loops_count; i++) {
        LoopDetail *loop = loop_analysis->loops[i];
        if (loop->depth != 1 && loop->parent != NULL) continue;

        ComplexityClass total_c = _calculate_single_loop_complexity(loop);

        char desc[256];
        snprintf(desc, sizeof(desc), "%s loop at line %d -> %s iterations",
                 loop->kind == LOOP_FOR ? "For" :
                 loop->kind == LOOP_WHILE ? "While" : "Do-while",
                 loop->lineno, complexity_class_to_string(total_c));
        _add_explanation(result, "loop", total_c, loop->lineno, desc, "");

        if (complexity_class_rank(total_c) > complexity_class_rank(max_complexity))
            max_complexity = total_c;
    }

    return max_complexity;
}

/* Estimate complexity contribution from recursion */
static ComplexityClass _estimate_recursion_complexity(RecursionInfo *info,
                                                        ComplexityResult *result)
{
    if (!info || !info->is_recursive)
        return COMPLEXITY_O_1;

    int branches = info->estimated_branches;
    const char *depth = info->depth_pattern;

    ComplexityClass complexity = COMPLEXITY_O_N;

    if (strcmp(depth, "logarithmic") == 0) {
        complexity = COMPLEXITY_O_LOG_N;
    } else if (strcmp(depth, "linear") == 0) {
        if (branches <= 1)
            complexity = COMPLEXITY_O_N;
        else if (branches == 2) {
            if (info->has_overlapping_subproblems)
                complexity = COMPLEXITY_O_2_N;
            else
                complexity = COMPLEXITY_O_N;
        } else {
            complexity = COMPLEXITY_O_2_N;
        }
    } else if (strcmp(depth, "exponential") == 0) {
        complexity = COMPLEXITY_O_2_N;
    }

    char desc[256];
    snprintf(desc, sizeof(desc),
             "Recursive function with %d branch(es), %s depth",
             branches, depth);
    if (info->can_be_memoized)
        strcat(desc, " (memoizable)");

    _add_explanation(result, "recursion", complexity, info->lineno, desc,
                     info->memoization_reason ? info->memoization_reason : "");

    if (!info->has_base_case) {
        char warn[256];
        snprintf(warn, sizeof(warn),
                 "Function '%s' at line %d: No clear base case detected.",
                 info->function_name, info->lineno);
        _add_warning(result, warn);
    }

    if (info->can_be_memoized) {
        char warn[256];
        snprintf(warn, sizeof(warn),
                 "Function '%s' at line %d: %s",
                 info->function_name, info->lineno,
                 info->memoization_reason ? info->memoization_reason :
                 "Memoization can improve performance.");
        _add_warning(result, warn);
    }

    return complexity;
}

ComplexityResult *estimate_complexity(CioptNode *func_node,
                                       const char *const *all_function_names,
                                       size_t function_count)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    const char *func_name = func_node->data.func_def.name;
    ComplexityResult *result = complexity_result_create(func_name, func_node->lineno);
    if (!result) return NULL;

    result->end_lineno = func_node->end_lineno;

    /* Step 1: Loop analysis */
    result->loop_analysis = detect_loops(func_node);
    ComplexityClass loop_complexity = _estimate_loop_complexity(result->loop_analysis, result);

    /* Step 2: Recursion analysis */
    result->recursion_info = detect_recursion(func_node, all_function_names, function_count);
    ComplexityClass recursion_complexity = _estimate_recursion_complexity(
        result->recursion_info, result);

    /* Step 3: Known function call analysis (in the body, not in loops) */
    ComplexityClass call_complexity = COMPLEXITY_O_1;
    _analyze_body_calls(func_node->data.func_def.body, result, &call_complexity);

    /* Step 4: Combine — take the max of all three */
    ComplexityClass overall = combine_complexities(
        combine_complexities(loop_complexity, recursion_complexity, "max"),
        call_complexity, "max");
    result->estimated_complexity = overall;

    /* Step 5: Confidence - higher confidence when analysis is clearer */
    if (result->loop_analysis->total_loops == 0 &&
        (!result->recursion_info || !result->recursion_info->is_recursive))
        result->confidence = 0.95;  /* Simple code, high confidence */
    else if (result->loop_analysis->max_depth <= 2 &&
             (!result->recursion_info || !result->recursion_info->is_recursive))
        result->confidence = 0.85;  /* Shallow loops, good confidence */
    else if (result->recursion_info && result->recursion_info->is_recursive &&
             result->recursion_info->has_base_case &&
             result->recursion_info->has_halving_pattern)
        result->confidence = 0.80;  /* Clear recursive pattern with base case and halving */
    else if (result->recursion_info && result->recursion_info->is_recursive &&
             result->recursion_info->has_base_case)
        result->confidence = 0.70;  /* Recursive with base case */
    else if (result->recursion_info && result->recursion_info->is_recursive)
        result->confidence = 0.50;  /* Recursive without clear base case */
    else
        result->confidence = 0.75;  /* Default */

    /* Bottleneck */
    if (result->explanations_count > 0) {
        int worst_idx = 0;
        for (size_t i = 1; i < result->explanations_count; i++) {
            if (complexity_class_rank(result->explanations[i].complexity) >
                complexity_class_rank(result->explanations[worst_idx].complexity))
                worst_idx = (int)i;
        }
        result->bottleneck_lines = (int *)malloc(sizeof(int));
        if (result->bottleneck_lines) {
            result->bottleneck_lines[0] = result->explanations[worst_idx].lineno;
            result->bottleneck_lines_count = 1;
        }
        result->bottleneck_description = strdup(
            result->explanations[worst_idx].description);
    }

    return result;
}

/* =========================================================================
 * Public API to add explanations (used for Call Graph resolution)
 * ========================================================================= */
void complexity_result_add_explanation(ComplexityResult *r, const char *source,
                                       ComplexityClass c, int lineno,
                                       const char *desc, const char *detail)
{
    _add_explanation(r, source, c, lineno, desc, detail);
}