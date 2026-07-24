#ifndef CIOPT_DATA_STRUCTURE_ANALYZER_H
#define CIOPT_DATA_STRUCTURE_ANALYZER_H

#include <stddef.h>
#include "../parser/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *variable_name;
    char *current_type;
    char *suggested_type;
    int lineno;
    char *description;
    char *suggestion;
    char *estimated_impact;
} DataStructureIssue;

typedef struct {
    char *function_name;
    DataStructureIssue *issues;
    size_t count;
    size_t capacity;
} DataStructureAnalysis;

DataStructureAnalysis *ds_analysis_create(const char *func_name);
void ds_analysis_free(DataStructureAnalysis *dsa);
int ds_analysis_add(DataStructureAnalysis *dsa, const char *var, const char *cur,
                     const char *sug, int lineno, const char *desc,
                     const char *suggestion, const char *impact);
DataStructureAnalysis *detect_data_structure_issues(CioptNode *func_node);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_DATA_STRUCTURE_ANALYZER_H */