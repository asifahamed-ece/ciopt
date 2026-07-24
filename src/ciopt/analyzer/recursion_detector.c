#include "recursion_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RecursionInfo *recursion_info_create(const char *func_name, int lineno)
{
    RecursionInfo *info = (RecursionInfo *)calloc(1, sizeof(RecursionInfo));
    if (!info) return NULL;
    info->function_name = strdup(func_name);
    info->lineno = lineno;
    info->end_lineno = lineno;
    info->depth_pattern = strdup("linear");
    info->has_halving_pattern = false;  /* Initialize to false */
    return info;
}

void recursion_info_free(RecursionInfo *info)
{
    if (!info) return;
    free(info->function_name);
    free(info->direct_calls);
    for (size_t i = 0; i < info->mutual_callers_count; i++)
        free(info->mutual_callers[i]);
    free(info->mutual_callers);
    free(info->mutual_lines);
    free(info->base_case_lines);
    free(info->tail_recursive_lines);
    free(info->memoization_reason);
    free(info->depth_pattern);
    free(info);
}

/* Count recursive calls in a node */
static int _count_recursive_calls(CioptNode *node, const char *func_name)
{
    if (!node) return 0;
    int count = 0;

    if (node->type == CIOPT_NODE_CALL) {
        const char *call_name = node->data.call.name;
        if (!call_name && node->data.call.callee && 
            node->data.call.callee->type == CIOPT_NODE_IDENTIFIER)
            call_name = node->data.call.callee->data.identifier.name;
        
        if (call_name && strcmp(call_name, func_name) == 0)
            count++;
    }

    /* Recurse into children */
    switch (node->type) {
        case CIOPT_NODE_FUNCTION_DEF:
            count += _count_recursive_calls(node->data.func_def.body, func_name);
            break;
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                count += _count_recursive_calls(node->data.block.stmts.nodes[i], func_name);
            break;
        case CIOPT_NODE_IF:
            count += _count_recursive_calls(node->data.if_stmt.then_body, func_name);
            count += _count_recursive_calls(node->data.if_stmt.else_body, func_name);
            break;
        case CIOPT_NODE_FOR:
            count += _count_recursive_calls(node->data.for_loop.body, func_name);
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            count += _count_recursive_calls(node->data.loop.body, func_name);
            break;
        case CIOPT_NODE_RETURN:
            count += _count_recursive_calls(node->data.return_stmt.value, func_name);
            break;
        case CIOPT_NODE_EXPR_STMT:
            count += _count_recursive_calls(node->data.expr_stmt.expr, func_name);
            break;
        case CIOPT_NODE_BINARY_OP:
            count += _count_recursive_calls(node->data.binary_op.left, func_name);
            count += _count_recursive_calls(node->data.binary_op.right, func_name);
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++)
                count += _count_recursive_calls(node->data.call.args.nodes[i], func_name);
            break;
        default:
            break;
    }

    return count;
}

/* Check if a node contains a recursive call */
static bool _has_recursive_call(CioptNode *node, const char *func_name)
{
    return _count_recursive_calls(node, func_name) > 0;
}

/* Check for base case: if branch that returns without recursing */
static void _check_base_cases(CioptNode *node, const char *func_name,
                               RecursionInfo *info)
{
    if (!node) return;

    if (node->type == CIOPT_NODE_FUNCTION_DEF) {
        _check_base_cases(node->data.func_def.body, func_name, info);
        return;
    }

    if (node->type == CIOPT_NODE_IF) {
        bool then_has_recursive = _has_recursive_call(node->data.if_stmt.then_body, func_name);
        bool else_has_recursive = node->data.if_stmt.else_body ?
            _has_recursive_call(node->data.if_stmt.else_body, func_name) : false;

        /* If one branch has no recursive call, it's a base case */
        if (node->data.if_stmt.then_body && !then_has_recursive) {
            info->has_base_case = true;
            /* Add line to base_case_lines */
            int *new_lines = (int *)realloc(info->base_case_lines,
                (info->base_case_lines_count + 1) * sizeof(int));
            if (new_lines) {
                info->base_case_lines = new_lines;
                info->base_case_lines[info->base_case_lines_count++] = node->lineno;
            }
        }
        if (node->data.if_stmt.else_body && !else_has_recursive) {
            info->has_base_case = true;
            int *new_lines = (int *)realloc(info->base_case_lines,
                (info->base_case_lines_count + 1) * sizeof(int));
            if (new_lines) {
                info->base_case_lines = new_lines;
                info->base_case_lines[info->base_case_lines_count++] = node->lineno;
            }
        }

        /* Recurse into branches */
        _check_base_cases(node->data.if_stmt.then_body, func_name, info);
        _check_base_cases(node->data.if_stmt.else_body, func_name, info);
    } else if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++)
            _check_base_cases(node->data.block.stmts.nodes[i], func_name, info);
    }
}

/* Check for tail recursion: return func(args) pattern */
static void _check_tail_recursion(CioptNode *node, const char *func_name,
                                   RecursionInfo *info)
{
    if (!node) return;

    if (node->type == CIOPT_NODE_FUNCTION_DEF) {
        _check_tail_recursion(node->data.func_def.body, func_name, info);
        return;
    }

    if (node->type == CIOPT_NODE_RETURN && node->data.return_stmt.value) {
        CioptNode *val = node->data.return_stmt.value;
        if (val->type == CIOPT_NODE_CALL && val->data.call.name &&
            strcmp(val->data.call.name, func_name) == 0) {
            info->is_tail_recursive = true;
            int *new_lines = (int *)realloc(info->tail_recursive_lines,
                (info->tail_recursive_lines_count + 1) * sizeof(int));
            if (new_lines) {
                info->tail_recursive_lines = new_lines;
                info->tail_recursive_lines[info->tail_recursive_lines_count++] = node->lineno;
            }
        }
    }

    /* Recurse */
    if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++)
            _check_tail_recursion(node->data.block.stmts.nodes[i], func_name, info);
    }
}

/* Check if an expression contains division by constant */
static bool _has_division_by_constant(CioptNode *node, int divisor)
{
    if (!node) return false;
    
    /* Direct match: division by the specified divisor */
    if (node->type == CIOPT_NODE_BINARY_OP && strcmp(node->data.binary_op.op, "/") == 0) {
        if (node->data.binary_op.right && node->data.binary_op.right->type == CIOPT_NODE_INT_LITERAL) {
            if (node->data.binary_op.right->data.int_literal.value == divisor)
                return true;
        }
        /* Only count as halving if it's actually dividing by the divisor (e.g., 2) */
        return false;
    }
    
    /* Recurse */
    switch (node->type) {
        case CIOPT_NODE_BINARY_OP:
            if (_has_division_by_constant(node->data.binary_op.left, divisor)) return true;
            if (_has_division_by_constant(node->data.binary_op.right, divisor)) return true;
            break;
        case CIOPT_NODE_ASSIGNMENT:
            if (_has_division_by_constant(node->data.assignment.value, divisor)) return true;
            break;
        case CIOPT_NODE_RETURN:
            if (_has_division_by_constant(node->data.return_stmt.value, divisor)) return true;
            break;
        case CIOPT_NODE_VARIABLE_DECL:
            if (_has_division_by_constant(node->data.var_decl.init, divisor)) return true;
            break;
        case CIOPT_NODE_EXPR_STMT:
            if (_has_division_by_constant(node->data.expr_stmt.expr, divisor)) return true;
            break;
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                if (_has_division_by_constant(node->data.block.stmts.nodes[i], divisor))
                    return true;
            break;
        case CIOPT_NODE_IF:
            if (_has_division_by_constant(node->data.if_stmt.then_body, divisor)) return true;
            if (_has_division_by_constant(node->data.if_stmt.else_body, divisor)) return true;
            break;
        case CIOPT_NODE_FOR:
            if (_has_division_by_constant(node->data.for_loop.body, divisor)) return true;
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            if (_has_division_by_constant(node->data.loop.body, divisor)) return true;
            break;
        default:
            break;
    }
    return false;
}

/* Analyze depth pattern */
static void _analyze_depth_pattern(CioptNode *node, const char *func_name,
                                    RecursionInfo *info)
{
    if (!node) return;

    if (node->type == CIOPT_NODE_FUNCTION_DEF) {
        _analyze_depth_pattern(node->data.func_def.body, func_name, info);
        return;
    }

    /* Check for halving patterns in the entire function body (binary search) */
    if (_has_division_by_constant(node, 2) && info->is_recursive) {
        free(info->depth_pattern);
        info->depth_pattern = strdup("logarithmic");
        info->has_halving_pattern = true;
        return;
    }

    /* Recurse */
    switch (node->type) {
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                _analyze_depth_pattern(node->data.block.stmts.nodes[i], func_name, info);
            break;
        case CIOPT_NODE_IF:
            _analyze_depth_pattern(node->data.if_stmt.then_body, func_name, info);
            _analyze_depth_pattern(node->data.if_stmt.else_body, func_name, info);
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++)
                _analyze_depth_pattern(node->data.call.args.nodes[i], func_name, info);
            break;
        default:
            break;
    }
}

/* Check if function body contains halving arithmetic */
static bool _function_has_halving(CioptNode *func_node)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return false;
    CioptNode *body = func_node->data.func_def.body;
    if (!body) return false;

    /* Search the entire function body for any division */
    return _has_division_by_constant(body, 2);
}

RecursionInfo *detect_recursion(CioptNode *func_node,
                                 const char *const *all_function_names,
                                 size_t function_count)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    (void)all_function_names;
    (void)function_count;

    const char *func_name = func_node->data.func_def.name;
    RecursionInfo *info = recursion_info_create(func_name, func_node->lineno);
    if (!info) return NULL;

    info->end_lineno = func_node->end_lineno;

    /* Count direct recursive calls */
    int total_calls = _count_recursive_calls(func_node, func_name);
    if (total_calls > 0) {
        info->is_recursive = true;

        /* Store direct call lines (simplified: just store one entry) */
        info->direct_calls = (int *)malloc(sizeof(int));
        if (info->direct_calls) {
            info->direct_calls[0] = func_node->lineno;
            info->direct_calls_count = 1;
        }

        /* Estimate branches */
        info->estimated_branches = total_calls;

        /* Check base cases */
        _check_base_cases(func_node, func_name, info);

        /* Check tail recursion */
        _check_tail_recursion(func_node, func_name, info);
        
        /* Check for halving/divide-and-conquer pattern (e.g., binary search) */
        if (total_calls > 0 && _function_has_halving(func_node)) {
            free(info->depth_pattern);
            info->depth_pattern = strdup("logarithmic");
        } else if (total_calls >= 2) {
            free(info->depth_pattern);
            info->depth_pattern = strdup("exponential");
            info->has_overlapping_subproblems = true;
            info->can_be_memoized = true;
            info->memoization_reason = strdup(
                "Function has multiple recursive calls with overlapping subproblems. "
                "Memoization can reduce exponential to polynomial complexity."
            );
        }
        _analyze_depth_pattern(func_node, func_name, info);
    }

    return info;
}