#ifndef CIOPT_COMPLEXITY_ESTIMATOR_H
#define CIOPT_COMPLEXITY_ESTIMATOR_H

#include <stdbool.h>
#include <stddef.h>
#include "../parser/ast.h"
#include "../config.h"
#include "loop_detector.h"
#include "recursion_detector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Big-O complexity estimation engine.
 *
 * Estimates time complexity for C functions using:
 * - AST-based loop nesting analysis
 * - Recursion pattern detection
 * - Built-in function complexity knowledge (qsort, strlen, etc.)
 *============================================================================*/

/* Explanation for a complexity component */
typedef struct {
    char *source;       /* "loop", "recursion", "call" */
    ComplexityClass complexity;
    int lineno;
    char *description;
    char *detail;
} ComplexityExplanation;
typedef struct {
    char *function_name;
    int lineno;
    int end_lineno;
    ComplexityClass estimated_complexity;
    double confidence;          /* 0.0 (guess) to 1.0 (certain) */
    ComplexityExplanation *explanations;
    size_t explanations_count;
    LoopAnalysis *loop_analysis;
    RecursionInfo *recursion_info;
    int *bottleneck_lines;
    size_t bottleneck_lines_count;
    char *bottleneck_description;
    char **warnings;
    size_t warnings_count;
} ComplexityResult;

/* Create a ComplexityResult */
ComplexityResult *complexity_result_create(const char *func_name, int lineno);

/* Free a ComplexityResult */
void complexity_result_free(ComplexityResult *result);

/* Combine two complexity classes.
 * For nested loops: multiply (O(n) * O(n) = O(n²))
 * For sequential code: take the max. */
ComplexityClass combine_complexities(ComplexityClass c1, ComplexityClass c2,
                                      const char *operation);

/* Estimate the Big-O time complexity of a function.
 * func_node: The function AST node to analyze.
 * all_function_names: NULL-terminated array of all function names for recursion analysis.
 * function_count: Number of function names. */
ComplexityResult *estimate_complexity(CioptNode *func_node,
                                       const char *const *all_function_names,
                                       size_t function_count);

/* Known C function complexities */
ComplexityClass get_known_function_complexity(const char *func_name);

/* Public API to add explanations (used for Call Graph resolution) */
void complexity_result_add_explanation(ComplexityResult *r, const char *source,
                                       ComplexityClass c, int lineno,
                                       const char *desc, const char *detail);                                       
/* Result of complexity estimation for a single function */

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_COMPLEXITY_ESTIMATOR_H */