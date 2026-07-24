#include "dead_code_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DeadCodeAnalysis *dead_code_analysis_create(void)
{
    return (DeadCodeAnalysis *)calloc(1, sizeof(DeadCodeAnalysis));
}

void dead_code_analysis_free(DeadCodeAnalysis *dca)
{
    if (!dca) return;
    for (size_t i = 0; i < dca->count; i++) {
        free(dca->items[i].kind);
        free(dca->items[i].name);
        free(dca->items[i].description);
        free(dca->items[i].suggestion);
    }
    free(dca->items);
    free(dca);
}

int dead_code_analysis_add(DeadCodeAnalysis *dca, const char *kind,
                            int lineno, int end_lineno, const char *name,
                            const char *desc, const char *suggestion)
{
    if (!dca) return -1;
    if (dca->count >= dca->capacity) {
        size_t new_cap = dca->capacity ? dca->capacity * 2 : 8;
        DeadCodeItem *new_i = (DeadCodeItem *)realloc(dca->items,
            new_cap * sizeof(DeadCodeItem));
        if (!new_i) return -1;
        dca->items = new_i;
        dca->capacity = new_cap;
    }
    DeadCodeItem *item = &dca->items[dca->count++];
    item->kind = strdup(kind);
    item->lineno = lineno;
    item->end_lineno = end_lineno;
    item->name = name ? strdup(name) : NULL;
    item->description = strdup(desc);
    item->suggestion = strdup(suggestion);

    if (strcmp(kind, "unreachable") == 0) dca->unreachable_count++;
    else if (strcmp(kind, "unused_variable") == 0) dca->unused_variable_count++;
    return 0;
}

/* Check for unreachable code after return/break/continue/goto */
static void _check_unreachable(CioptNode *node, DeadCodeAnalysis *dca)
{
    if (!node || node->type != CIOPT_NODE_BLOCK) return;

    for (size_t i = 0; i < node->data.block.stmts.count; i++) {
        CioptNode *stmt = node->data.block.stmts.nodes[i];
        if (ciopt_node_is_jump(stmt->type)) {
            /* Everything after this is unreachable */
            for (size_t j = i + 1; j < node->data.block.stmts.count; j++) {
                CioptNode *dead = node->data.block.stmts.nodes[j];
                dead_code_analysis_add(dca, "unreachable",
                    dead->lineno, dead->end_lineno, "",
                    "Unreachable code after jump statement",
                    "Remove unreachable code or restructure control flow.");
            }
            break;
        }
    }
}

DeadCodeAnalysis *detect_dead_code(CioptNode *func_node)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    DeadCodeAnalysis *dca = dead_code_analysis_create();
    if (!dca) return NULL;

    /* Check for unreachable code in the function body */
    if (func_node->data.func_def.body)
        _check_unreachable(func_node->data.func_def.body, dca);

    return dca;
}