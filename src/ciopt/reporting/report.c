#include "report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FunctionReport *function_report_create(const char *name, int lineno, int end_lineno)
{
    FunctionReport *fr = (FunctionReport *)calloc(1, sizeof(FunctionReport));
    if (!fr) return NULL;
    fr->name = strdup(name);
    fr->lineno = lineno;
    fr->end_lineno = end_lineno;
    fr->line_count = end_lineno - lineno + 1;
    fr->severity = SEVERITY_INFO;
    return fr;
}

void function_report_free(FunctionReport *fr)
{
    if (!fr) return;
    free(fr->name);
    if (fr->complexity) complexity_result_free(fr->complexity);
    if (fr->patterns) pattern_analysis_free(fr->patterns);
    if (fr->data_structure) ds_analysis_free(fr->data_structure);
    free(fr);
}

FileReport *file_report_create(const char *filepath, int line_count)
{
    FileReport *fr = (FileReport *)calloc(1, sizeof(FileReport));
    if (!fr) return NULL;
    fr->filepath = strdup(filepath);
    fr->line_count = line_count;
    return fr;
}

void file_report_free(FileReport *fr)
{
    if (!fr) return;
    free(fr->filepath);
    for (size_t i = 0; i < fr->functions_count; i++)
        function_report_free(fr->functions[i]);
    free(fr->functions);
    if (fr->dead_code) dead_code_analysis_free(fr->dead_code);
    for (size_t i = 0; i < fr->parse_errors_count; i++)
        free(fr->parse_errors[i]);
    free(fr->parse_errors);
    free(fr);
}

AnalysisReport *analysis_report_create(void)
{
    AnalysisReport *ar = (AnalysisReport *)calloc(1, sizeof(AnalysisReport));
    if (!ar) return NULL;

    /* Generate timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", tm_info);
    ar->timestamp = strdup(ts);
    ar->ciopt_version = strdup("0.1.0");
    ar->analysis_duration_ms = 0.0;
    return ar;
}

void analysis_report_free(AnalysisReport *ar)
{
    if (!ar) return;
    for (size_t i = 0; i < ar->files_count; i++)
        file_report_free(ar->files[i]);
    free(ar->files);
    free(ar->timestamp);
    free(ar->ciopt_version);
    free(ar);
}

int file_report_add_function(FileReport *fr, FunctionReport *func)
{
    if (!fr || !func) return -1;
    if (fr->functions_count >= fr->functions_capacity) {
        size_t new_cap = fr->functions_capacity ? fr->functions_capacity * 2 : 16;
        FunctionReport **new_f = (FunctionReport **)realloc(fr->functions,
            new_cap * sizeof(FunctionReport *));
        if (!new_f) return -1;
        fr->functions = new_f;
        fr->functions_capacity = new_cap;
    }
    fr->functions[fr->functions_count++] = func;
    return 0;
}

int analysis_report_add_file(AnalysisReport *ar, FileReport *fr)
{
    if (!ar || !fr) return -1;
    if (ar->files_count >= ar->files_capacity) {
        size_t new_cap = ar->files_capacity ? ar->files_capacity * 2 : 16;
        FileReport **new_f = (FileReport **)realloc(ar->files,
            new_cap * sizeof(FileReport *));
        if (!new_f) return -1;
        ar->files = new_f;
        ar->files_capacity = new_cap;
    }
    ar->files[ar->files_count++] = fr;
    return 0;
}

int file_report_add_error(FileReport *fr, const char *error)
{
    if (!fr || !error) return -1;
    char **new_e = (char **)realloc(fr->parse_errors,
        (fr->parse_errors_count + 1) * sizeof(char *));
    if (!new_e) return -1;
    fr->parse_errors = new_e;
    fr->parse_errors[fr->parse_errors_count++] = strdup(error);
    return 0;
}

Severity determine_severity(FunctionReport *func, AnalysisConfig *config)
{
    if (!func || !func->complexity) return SEVERITY_INFO;

    ComplexityClass c = func->complexity->estimated_complexity;

    if (complexity_class_compare(c, config->complexity_critical_threshold) >= 0)
        return SEVERITY_CRITICAL;
    if (complexity_class_compare(c, config->complexity_warning_threshold) >= 0)
        return SEVERITY_WARNING;

    if (func->patterns) {
        if (func->patterns->critical_count > 0) return SEVERITY_CRITICAL;
        if (func->patterns->warning_count > 0) return SEVERITY_WARNING;
    }

    if (func->complexity->warnings_count > 0)
        return SEVERITY_WARNING;

    return SEVERITY_INFO;
}

ComplexityClass analysis_worst_complexity(AnalysisReport *ar)
{
    if (!ar || ar->files_count == 0) return COMPLEXITY_O_1;
    ComplexityClass worst = COMPLEXITY_O_1;
    for (size_t f = 0; f < ar->files_count; f++) {
        for (size_t i = 0; i < ar->files[f]->functions_count; i++) {
            if (ar->files[f]->functions[i]->complexity) {
                ComplexityClass c = ar->files[f]->functions[i]->complexity->estimated_complexity;
                if (complexity_class_compare(c, worst) > 0)
                    worst = c;
            }
        }
    }
    return worst;
}

int analysis_total_issues(AnalysisReport *ar)
{
    if (!ar) return 0;
    int total = 0;
    for (size_t f = 0; f < ar->files_count; f++) {
        FileReport *fr = ar->files[f];
        for (size_t i = 0; i < fr->functions_count; i++) {
            FunctionReport *func = fr->functions[i];
            if (func->patterns) total += (int)func->patterns->count;
            if (func->complexity) total += (int)func->complexity->warnings_count;
        }
        if (fr->dead_code) total += (int)fr->dead_code->count;
    }
    return total;
}

int analysis_total_functions(AnalysisReport *ar)
{
    if (!ar) return 0;
    int total = 0;
    for (size_t f = 0; f < ar->files_count; f++)
        total += (int)ar->files[f]->functions_count;
    return total;
}