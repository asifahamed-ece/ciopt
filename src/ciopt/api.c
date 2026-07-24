#include "api.h"
#include "parser/source_loader.h"
#include "parser/ts_parser.h"
#include "analyzer/complexity_estimator.h"
#include "analyzer/pattern_matcher.h"
#include "analyzer/dead_code_detector.h"
#include "analyzer/data_structure_analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Collect function names from AST for recursion analysis */
static void _collect_func_names(CioptNode *node, char ***names, size_t *count, size_t *cap)
{
    if (!node) return;

    if (node->type == CIOPT_NODE_FUNCTION_DEF && node->data.func_def.name) {
        if (*count >= *cap) {
            *cap = *cap ? *cap * 2 : 16;
            char **new_n = (char **)realloc(*names, *cap * sizeof(char *));
            if (!new_n) return;
            *names = new_n;
        }
        (*names)[*count] = strdup(node->data.func_def.name);
        (*count)++;
    }

    /* Recurse */
    switch (node->type) {
        case CIOPT_NODE_PROGRAM:
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                _collect_func_names(node->data.block.stmts.nodes[i], names, count, cap);
            break;
        case CIOPT_NODE_FUNCTION_DEF:
            if (node->data.func_def.body)
                _collect_func_names(node->data.func_def.body, names, count, cap);
            break;
        default:
            break;
    }
}

/* Analyze a function AST node and produce a FunctionReport */
static FunctionReport *_analyze_function(CioptNode *func_node,
                                           AnalysisConfig *config,
                                           const char *const *all_func_names,
                                           size_t func_count)
{
    if (!func_node || func_node->type != CIOPT_NODE_FUNCTION_DEF) return NULL;

    FunctionReport *fr = function_report_create(
        func_node->data.func_def.name,
        func_node->lineno,
        func_node->end_lineno);
    if (!fr) return NULL;

    /* Complexity estimation */
    fr->complexity = estimate_complexity(func_node, all_func_names, func_count);

    /* Anti-pattern detection */
    if (config->detect_anti_patterns)
        fr->patterns = detect_patterns(func_node);

    /* Data structure analysis */
    if (config->detect_data_structure_issues)
        fr->data_structure = detect_data_structure_issues(func_node);

    /* Severity */
    fr->severity = determine_severity(fr, config);

    return fr;
}

/* Analyze a single source file */
static FileReport *_analyze_file(SourceFile *sf, AnalysisConfig *config)
{
    if (!sf || !config) return NULL;

    FileReport *fr = file_report_create(sf->path, sf->line_count);
    if (!fr) return NULL;

    /* Parse with Tree-sitter */
    CioptNode *ast = ts_parse_c(sf->content, sf->path);
    if (!ast) {
        file_report_add_error(fr, "Failed to parse file with Tree-sitter");
        return fr;
    }

    /* Collect all function names for recursion analysis */
    char **all_func_names = NULL;
    size_t func_count = 0, func_cap = 0;
    _collect_func_names(ast, &all_func_names, &func_count, &func_cap);
    all_func_names = (char **)realloc(all_func_names, (func_count + 1) * sizeof(char *));
    if (all_func_names) all_func_names[func_count] = NULL;

    /* Walk top-level statements looking for function definitions */
    for (size_t i = 0; i < ast->data.block.stmts.count; i++) {
        CioptNode *stmt = ast->data.block.stmts.nodes[i];
        if (stmt->type == CIOPT_NODE_FUNCTION_DEF) {
            FunctionReport *func_report = _analyze_function(
                stmt, config,
                (const char *const *)all_func_names, func_count);
            if (func_report)
                file_report_add_function(fr, func_report);
        }
    }

    /* Dead code detection */
    if (config->detect_dead_code) {
        /* Run on each function for unreachable code */
        for (size_t i = 0; i < ast->data.block.stmts.count; i++) {
            CioptNode *stmt = ast->data.block.stmts.nodes[i];
            if (stmt->type == CIOPT_NODE_FUNCTION_DEF) {
                DeadCodeAnalysis *dca = detect_dead_code(stmt);
                if (dca) {
                    if (!fr->dead_code) {
                        fr->dead_code = dca;
                    } else {
                        for (size_t k = 0; k < dca->count; k++) {
                            dead_code_analysis_add(fr->dead_code,
                                dca->items[k].kind,
                                dca->items[k].lineno,
                                dca->items[k].end_lineno,
                                dca->items[k].name,
                                dca->items[k].description,
                                dca->items[k].suggestion);
                        }
                        dead_code_analysis_free(dca);
                    }
                }
            }
        }
    }

    /* Cleanup */
    for (size_t i = 0; i < func_count; i++)
        free(all_func_names[i]);
    free(all_func_names);
    ciopt_node_free(ast);

    return fr;
}

AnalysisReport *ciopt_analyze(const char *path, AnalysisConfig *config)
{
    if (!path) return NULL;
    if (!config) config = config_create_default();

    double start_time = (double)clock() / CLOCKS_PER_SEC;
    AnalysisReport *report = analysis_report_create();
    if (!report) return NULL;

    /* Check if path is a file or directory */
    SourceFile *sf = source_load(path);
    if (sf) {
        FileReport *fr = _analyze_file(sf, config);
        if (fr) analysis_report_add_file(report, fr);
        source_free(sf);
    } else {
        /* Try as directory */
        SourceFile **files = source_scan_directory(path,
            (const char *const *)config->file_extensions,
            config->file_extensions_count,
            (const char *const *)config->exclude_dirs,
            config->exclude_dirs_count);
        if (files) {
            for (size_t i = 0; files[i] != NULL; i++) {
                FileReport *fr = _analyze_file(files[i], config);
                if (fr) analysis_report_add_file(report, fr);
            }
            source_scan_free(files);
        }
    }

    double end_time = (double)clock() / CLOCKS_PER_SEC;
    report->analysis_duration_ms = (end_time - start_time) * 1000.0;

    return report;
}

AnalysisReport *ciopt_analyze_source(const char *source, const char *filename,
                                      AnalysisConfig *config)
{
    if (!source) return NULL;
    if (!config) config = config_create_default();

    double start_time = (double)clock() / CLOCKS_PER_SEC;
    AnalysisReport *report = analysis_report_create();
    if (!report) return NULL;

    SourceFile *sf = source_from_string(source, filename);
    if (sf) {
        FileReport *fr = _analyze_file(sf, config);
        if (fr) analysis_report_add_file(report, fr);
        source_free(sf);
    }

    double end_time = (double)clock() / CLOCKS_PER_SEC;
    report->analysis_duration_ms = (end_time - start_time) * 1000.0;

    return report;
}