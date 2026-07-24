#ifndef CIOPT_LOOP_DETECTOR_H
#define CIOPT_LOOP_DETECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include "../parser/ast.h"
#include "../config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Loop detection and nesting analysis for C code.
 *============================================================================*/

/* Loop kind */
typedef enum {
    LOOP_FOR,
    LOOP_WHILE,
    LOOP_DO_WHILE
} LoopKind;

/* Loop variable information */
typedef struct {
    char *name;         /* variable name */
    int lineno;
    char *iterable;     /* description of what's being iterated */
    bool is_range;      /* true if it's a for-loop with range-like pattern */
    bool has_constant_bound;  /* e.g., for (int i = 0; i < 10; i++) */
    long long constant_bound; /* the constant bound if has_constant_bound */
    bool has_variable_bound;  /* e.g., for (int i = 0; i < n; i++) */
    char *variable_bound;     /* the variable name for the bound */
    bool is_halving;          /* n /= 2 pattern */
    bool is_doubling;         /* n *= 2 pattern */
} LoopVariable;

/* Forward declaration */
typedef struct LoopDetail LoopDetail;

/* Loop detail */
struct LoopDetail {
    LoopKind kind;
    int lineno;
    int end_lineno;
    int depth;                  /* 1 = top-level, 2 = nested, etc. */
    LoopDetail *parent;
    LoopDetail **children;
    size_t children_count;
    size_t children_capacity;
    LoopVariable *variables;
    size_t variables_count;
    size_t variables_capacity;
    bool contains_break;
    bool contains_continue;
    bool contains_return;
    bool has_invariant_code;
    int *invariant_lines;
    size_t invariant_count;
    bool has_expensive_operation;
    char **expensive_operations;
    size_t expensive_count;
    size_t expensive_capacity;
    CioptNode *node;            /* pointer to AST node */
};

/* Loop analysis results */
typedef struct {
    char *function_name;
    LoopDetail **loops;
    size_t loops_count;
    size_t loops_capacity;
    int max_depth;
    int total_loops;
} LoopAnalysis;

/* Create a loop detail */
LoopDetail *loop_detail_create(LoopKind kind, int lineno);

/* Add a child loop */
int loop_detail_add_child(LoopDetail *parent, LoopDetail *child);

/* Add a loop variable */
int loop_detail_add_variable(LoopDetail *loop, const char *name, int lineno);

/* Free a loop detail */
void loop_detail_free(LoopDetail *loop);

/* Create loop analysis */
LoopAnalysis *loop_analysis_create(const char *function_name);

/* Free loop analysis */
void loop_analysis_free(LoopAnalysis *analysis);

/* Detect and analyze all loops in a function AST node.
 * Returns LoopAnalysis with detailed information. */
LoopAnalysis *detect_loops(CioptNode *func_node);

/* Estimate the iteration count complexity of a loop.
 * Returns the complexity class. */
ComplexityClass estimate_loop_iterations(LoopDetail *loop);

/* Check if a while loop has a halving pattern (binary search). */
bool check_halving_pattern(CioptNode *node);

/* Check if a while loop has a doubling pattern (exponential). */
bool check_doubling_pattern(CioptNode *node);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_LOOP_DETECTOR_H */