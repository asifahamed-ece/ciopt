#ifndef CIOPT_PATTERN_MATCHER_H
#define CIOPT_PATTERN_MATCHER_H

#include <stdbool.h>
#include <stddef.h>
#include "../parser/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Algorithmic anti-pattern detection for C code.
 *============================================================================*/

typedef enum {
    PATTERN_INFO = 0,
    PATTERN_WARNING,
    PATTERN_CRITICAL
} PatternSeverity;

typedef enum {
    CATEGORY_DATA_STRUCTURE,
    CATEGORY_STRING_OPERATION,
    CATEGORY_ALGORITHM,
    CATEGORY_MEMORY,
    CATEGORY_COMPUTATION,
    CATEGORY_SECURITY
} PatternCategory;

typedef struct {
    char *name;
    PatternCategory category;
    PatternSeverity severity;
    int lineno;
    int end_lineno;
    char *description;
    char *suggestion;
    char *estimated_impact;
} AntiPattern;

typedef struct {
    char *function_name;
    AntiPattern *anti_patterns;
    size_t count;
    size_t capacity;
    int critical_count;
    int warning_count;
} PatternAnalysis;

/* Create PatternAnalysis */
PatternAnalysis *pattern_analysis_create(const char *func_name);

/* Free PatternAnalysis */
void pattern_analysis_free(PatternAnalysis *pa);

/* Add an anti-pattern */
int pattern_analysis_add(PatternAnalysis *pa, const char *name,
                          PatternCategory cat, PatternSeverity sev,
                          int lineno, int end_lineno,
                          const char *desc, const char *suggestion,
                          const char *impact);

/* Detect anti-patterns in a function AST node. */
PatternAnalysis *detect_patterns(CioptNode *func_node);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_PATTERN_MATCHER_H */