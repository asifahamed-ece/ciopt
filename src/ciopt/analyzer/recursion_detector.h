#ifndef CIOPT_RECURSION_DETECTOR_H
#define CIOPT_RECURSION_DETECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include "../parser/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Recursion detection and analysis for C code.
 *============================================================================*/

typedef struct {
    char *function_name;
    int lineno;
    int end_lineno;
    bool is_recursive;
    int *direct_calls;          /* line numbers of recursive calls */
    size_t direct_calls_count;
    char **mutual_callers;      /* functions that call us back */
    size_t mutual_callers_count;
    int *mutual_lines;          /* line numbers of mutual calls */
    size_t mutual_lines_count;
    bool has_base_case;
    int *base_case_lines;
    size_t base_case_lines_count;
    int estimated_branches;     /* how many recursive calls per invocation */
    bool is_tail_recursive;
    int *tail_recursive_lines;
    size_t tail_recursive_lines_count;
    bool can_be_memoized;
    char *memoization_reason;
    bool has_overlapping_subproblems;
    char *depth_pattern;        /* "linear", "logarithmic", "exponential" */
} RecursionInfo;

/* Create a RecursionInfo */
RecursionInfo *recursion_info_create(const char *func_name, int lineno);

/* Free RecursionInfo */
void recursion_info_free(RecursionInfo *info);

/* Detect and analyze recursion in a function AST node.
 * all_function_names: NULL-terminated array of all function names in the module.
 * Returns RecursionInfo with detailed analysis. */
RecursionInfo *detect_recursion(CioptNode *func_node,
                                 const char *const *all_function_names,
                                 size_t function_count);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_RECURSION_DETECTOR_H */