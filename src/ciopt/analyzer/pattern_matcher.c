#include "pattern_matcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PatternAnalysis *pattern_analysis_create(const char *func_name)
{
    PatternAnalysis *pa = (PatternAnalysis *)calloc(1, sizeof(PatternAnalysis));
    if (!pa) return NULL;
    pa->function_name = strdup(func_name);
    return pa;
}

void pattern_analysis_free(PatternAnalysis *pa)
{
    if (!pa) return;
    free(pa->function_name);
    for (size_t i = 0; i < pa->count; i++) {
        free(pa->anti_patterns[i].name);
        free(pa->anti_patterns[i].description);
        free(pa->anti_patterns[i].suggestion);
        free(pa->anti_patterns[i].estimated_impact);
    }
    free(pa->anti_patterns);
    free(pa);
}

int pattern_analysis_add(PatternAnalysis *pa, const char *name,
                          PatternCategory cat, PatternSeverity sev,
                          int lineno, int end_lineno,
                          const char *desc, const char *suggestion,
                          const char *impact)
{
    if (!pa) return -1;
    if (pa->count >= pa->capacity) {
        size_t new_cap = pa->capacity ? pa->capacity * 2 : 8;
        AntiPattern *new_p = (AntiPattern *)realloc(pa->anti_patterns,
            new_cap * sizeof(AntiPattern));
        if (!new_p) return -1;
        pa->anti_patterns = new_p;
        pa->capacity = new_cap;
    }
    AntiPattern *p = &pa->anti_patterns[pa->count++];
    p->name = strdup(name);
    p->category = cat;
    p->severity = sev;
    p->lineno = lineno;
    p->end_lineno = end_lineno;
    p->description = strdup(desc);
    p->suggestion = strdup(suggestion);
    p->estimated_impact = strdup(impact);

    if (sev == PATTERN_CRITICAL) pa->critical_count++;
    else if (sev == PATTERN_WARNING) pa->warning_count++;
    return 0;
}

/* Walk a node looking for anti-patterns */
static void _walk_patterns(CioptNode *node, PatternAnalysis *pa,
                            int loop_depth)
{
    if (!node) return;

    /* Check for strcat in loops */
    if (node->type == CIOPT_NODE_CALL && node->data.call.name &&
        strcmp(node->data.call.name, "strcat") == 0 && loop_depth > 0) {
        pattern_analysis_add(pa, "strcat_in_loop", CATEGORY_STRING_OPERATION,
            PATTERN_CRITICAL, node->lineno, node->end_lineno,
            "strcat() inside loop — O(n²) string concatenation",
            "Use a string buffer or snprintf to build strings efficiently.",
            "O(n²) → O(n)");
    }

    /* Check for realloc in loops */
    if (node->type == CIOPT_NODE_CALL && node->data.call.name &&
        strcmp(node->data.call.name, "realloc") == 0 && loop_depth > 0) {
        pattern_analysis_add(pa, "realloc_in_loop", CATEGORY_MEMORY,
            PATTERN_WARNING, node->lineno, node->end_lineno,
            "realloc() inside loop — may cause O(n²) memory operations",
            "Pre-allocate memory before the loop, or use a growth strategy.",
            "O(n²) → O(n)");
    }

    /* Check for strlen in loop condition */
    if (node->type == CIOPT_NODE_FOR) {
        CioptNode *cond = node->data.for_loop.condition;
        if (cond && cond->type == CIOPT_NODE_BINARY_OP) {
            /* Check for strlen() in condition */
            if (cond->data.binary_op.left &&
                cond->data.binary_op.left->type == CIOPT_NODE_CALL &&
                cond->data.binary_op.left->data.call.name &&
                strcmp(cond->data.binary_op.left->data.call.name, "strlen") == 0) {
                pattern_analysis_add(pa, "strlen_in_loop_condition",
                    CATEGORY_ALGORITHM, PATTERN_CRITICAL,
                    node->lineno, node->end_lineno,
                    "strlen() in for-loop condition — O(n²) total",
                    "Cache strlen result in a variable before the loop.",
                    "O(n²) → O(n)");
            }
        }
    }

    /* Check for gets() usage (security) */
    if (node->type == CIOPT_NODE_CALL && node->data.call.name &&
        strcmp(node->data.call.name, "gets") == 0) {
        pattern_analysis_add(pa, "unsafe_gets", CATEGORY_SECURITY,
            PATTERN_CRITICAL, node->lineno, node->end_lineno,
            "Unsafe gets() call — buffer overflow risk",
            "Use fgets() with a size limit instead.",
            "Security fix");
    }

    /* Recurse */
    switch (node->type) {
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                _walk_patterns(node->data.block.stmts.nodes[i], pa, loop_depth);
            break;
        case CIOPT_NODE_IF:
            _walk_patterns(node->data.if_stmt.then_body, pa, loop_depth);
            _walk_patterns(node->data.if_stmt.else_body, pa, loop_depth);
            break;
        case CIOPT_NODE_FOR:
            _walk_patterns(node->data.for_loop.body, pa, loop_depth + 1);
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            _walk_patterns(node->data.loop.body, pa, loop_depth + 1);
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++)
                _walk_patterns(node->data.call.args.nodes[i], pa, loop_depth);
            break;
        default:
            break;
    }
}

PatternAnalysis *detect_patterns(CioptNode *func_node)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    PatternAnalysis *pa = pattern_analysis_create(func_node->data.func_def.name);
    if (!pa) return NULL;

    _walk_patterns(func_node->data.func_def.body, pa, 0);
    return pa;
}