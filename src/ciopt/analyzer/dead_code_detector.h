#ifndef CIOPT_DEAD_CODE_DETECTOR_H
#define CIOPT_DEAD_CODE_DETECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include "../parser/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Dead code detection.
 *============================================================================*/

typedef struct {
    char *kind;         /* "unreachable", "unused_variable" */
    int lineno;
    int end_lineno;
    char *name;
    char *description;
    char *suggestion;
} DeadCodeItem;

typedef struct {
    DeadCodeItem *items;
    size_t count;
    size_t capacity;
    int unreachable_count;
    int unused_variable_count;
} DeadCodeAnalysis;

/* Create DeadCodeAnalysis */
DeadCodeAnalysis *dead_code_analysis_create(void);

/* Free DeadCodeAnalysis */
void dead_code_analysis_free(DeadCodeAnalysis *dca);

/* Add a dead code item */
int dead_code_analysis_add(DeadCodeAnalysis *dca, const char *kind,
                            int lineno, int end_lineno, const char *name,
                            const char *desc, const char *suggestion);

/* Detect dead code in a function AST node. */
DeadCodeAnalysis *detect_dead_code(CioptNode *func_node);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_DEAD_CODE_DETECTOR_H */