#ifndef CIOPT_REPORT_H
#define CIOPT_REPORT_H

#include <stdbool.h>
#include <stddef.h>
#include "../config.h"
#include "../analyzer/complexity_estimator.h"
#include "../analyzer/pattern_matcher.h"
#include "../analyzer/dead_code_detector.h"
#include "../analyzer/data_structure_analyzer.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Analysis report data model.
 *============================================================================*/

typedef struct {
    char *name;
    int lineno;
    int end_lineno;
    int line_count;
    ComplexityResult *complexity;
    PatternAnalysis *patterns;
    DataStructureAnalysis *data_structure;
    Severity severity;
} FunctionReport;

typedef struct {
    char *filepath;
    int line_count;
    FunctionReport **functions;
    size_t functions_count;
    size_t functions_capacity;
    DeadCodeAnalysis *dead_code;
    char **parse_errors;
    size_t parse_errors_count;
} FileReport;

typedef struct {
    FileReport **files;
    size_t files_count;
    size_t files_capacity;
    char *timestamp;
    double analysis_duration_ms;
    char *ciopt_version;
} AnalysisReport;

/* Create function report */
FunctionReport *function_report_create(const char *name, int lineno, int end_lineno);

/* Free function report */
void function_report_free(FunctionReport *fr);

/* Create file report */
FileReport *file_report_create(const char *filepath, int line_count);

/* Free file report */
void file_report_free(FileReport *fr);

/* Create analysis report */
AnalysisReport *analysis_report_create(void);

/* Free analysis report (frees all contained data) */
void analysis_report_free(AnalysisReport *ar);

/* Add function report to file report */
int file_report_add_function(FileReport *fr, FunctionReport *func);

/* Add file report to analysis report */
int analysis_report_add_file(AnalysisReport *ar, FileReport *fr);

/* Add parse error to file report */
int file_report_add_error(FileReport *fr, const char *error);

/* Compute severity for a function report */
Severity determine_severity(FunctionReport *func, AnalysisConfig *config);

/* Get worst complexity across all files */
ComplexityClass analysis_worst_complexity(AnalysisReport *ar);

/* Get total issues across all files */
int analysis_total_issues(AnalysisReport *ar);

/* Get total functions across all files */
int analysis_total_functions(AnalysisReport *ar);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_REPORT_H */