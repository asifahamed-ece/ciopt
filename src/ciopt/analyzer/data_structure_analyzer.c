#include "data_structure_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DataStructureAnalysis *ds_analysis_create(const char *func_name)
{
    DataStructureAnalysis *dsa = (DataStructureAnalysis *)calloc(1, sizeof(DataStructureAnalysis));
    if (!dsa) return NULL;
    dsa->function_name = strdup(func_name);
    return dsa;
}

void ds_analysis_free(DataStructureAnalysis *dsa)
{
    if (!dsa) return;
    free(dsa->function_name);
    for (size_t i = 0; i < dsa->count; i++) {
        free(dsa->issues[i].variable_name);
        free(dsa->issues[i].current_type);
        free(dsa->issues[i].suggested_type);
        free(dsa->issues[i].description);
        free(dsa->issues[i].suggestion);
        free(dsa->issues[i].estimated_impact);
    }
    free(dsa->issues);
    free(dsa);
}

int ds_analysis_add(DataStructureAnalysis *dsa, const char *var, const char *cur,
                     const char *sug, int lineno, const char *desc,
                     const char *suggestion, const char *impact)
{
    if (!dsa) return -1;
    if (dsa->count >= dsa->capacity) {
        size_t new_cap = dsa->capacity ? dsa->capacity * 2 : 8;
        DataStructureIssue *new_i = (DataStructureIssue *)realloc(dsa->issues,
            new_cap * sizeof(DataStructureIssue));
        if (!new_i) return -1;
        dsa->issues = new_i;
        dsa->capacity = new_cap;
    }
    DataStructureIssue *issue = &dsa->issues[dsa->count++];
    issue->variable_name = strdup(var);
    issue->current_type = strdup(cur);
    issue->suggested_type = strdup(sug);
    issue->lineno = lineno;
    issue->description = strdup(desc);
    issue->suggestion = strdup(suggestion);
    issue->estimated_impact = strdup(impact);
    return 0;
}

DataStructureAnalysis *detect_data_structure_issues(CioptNode *func_node)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    DataStructureAnalysis *dsa = ds_analysis_create(func_node->data.func_def.name);
    if (!dsa) return NULL;

    /* C-specific data structure analysis is limited without type information.
     * This is a placeholder that can be extended with Tree-sitter type queries. */
    return dsa;
}