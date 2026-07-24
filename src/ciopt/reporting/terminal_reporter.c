#include "terminal_reporter.h"
#include "../utils/string_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ANSI color codes */
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_DIM     "\x1b[2m"

static const char *_severity_color(Severity s)
{
    switch (s) {
        case SEVERITY_CRITICAL: return ANSI_RED ANSI_BOLD;
        case SEVERITY_WARNING:  return ANSI_YELLOW;
        case SEVERITY_INFO:     return ANSI_GREEN;
        default:                return ANSI_RESET;
    }
}

static const char *_severity_label(Severity s)
{
    switch (s) {
        case SEVERITY_CRITICAL: return "CRITICAL";
        case SEVERITY_WARNING:  return "WARNING";
        case SEVERITY_INFO:     return "INFO";
        default:                return "?";
    }
}

void render_terminal(AnalysisReport *report, bool verbose, const char *output_path)
{
    if (!report) return;

    FILE *out = stdout;
    if (output_path) {
        out = fopen(output_path, "w");
        if (!out) {
            fprintf(stderr, "Error: Cannot write to %s\n", output_path);
            return;
        }
    }

    /* Header */
    fprintf(out, ANSI_BOLD "CiOpt Analysis Report" ANSI_RESET "\n");
    fprintf(out, "%s\n", "==============================");
    fprintf(out, "Files analyzed: %d\n", analysis_total_functions(report) > 0 ?
            (int)report->files_count : 0);
    fprintf(out, "Functions analyzed: %d\n", analysis_total_functions(report));
    fprintf(out, "Worst complexity: %s\n",
            complexity_class_to_string(analysis_worst_complexity(report)));
    fprintf(out, "Total issues: %d\n", analysis_total_issues(report));
    fprintf(out, "Analysis time: %.1f ms\n\n", report->analysis_duration_ms);

    /* Per-file results */
    for (size_t f = 0; f < report->files_count; f++) {
        FileReport *fr = report->files[f];
        fprintf(out, ANSI_BOLD "File: %s" ANSI_RESET " (%d lines)\n",
                fr->filepath, fr->line_count);

        if (fr->parse_errors_count > 0) {
            for (size_t e = 0; e < fr->parse_errors_count; e++)
                fprintf(out, "  " ANSI_RED "Error: %s" ANSI_RESET "\n",
                        fr->parse_errors[e]);
        }

        for (size_t i = 0; i < fr->functions_count; i++) {
            FunctionReport *func = fr->functions[i];
            const char *color = _severity_color(func->severity);
            const char *label = _severity_label(func->severity);

            fprintf(out, "  %s[%s]%s %s (L%d-%d): %s\n",
                    color, label, ANSI_RESET,
                    func->name, func->lineno, func->end_lineno,
                    func->complexity ?
                        complexity_class_to_string(
                            func->complexity->estimated_complexity) : "Unknown");

            if (verbose && func->complexity) {
                /* Show explanations */
                for (size_t e = 0; e < func->complexity->explanations_count; e++) {
                    ComplexityExplanation *exp = &func->complexity->explanations[e];
                    fprintf(out, "    " ANSI_DIM "%s" ANSI_RESET " - %s\n",
                            complexity_class_to_string(exp->complexity),
                            exp->description);
                }

                /* Show warnings */
                for (size_t w = 0; w < func->complexity->warnings_count; w++) {
                    fprintf(out, "    " ANSI_YELLOW "⚠ %s" ANSI_RESET "\n",
                            func->complexity->warnings[w]);
                }
            }

            /* Show anti-patterns */
            if (func->patterns && func->patterns->count > 0) {
                for (size_t p = 0; p < func->patterns->count; p++) {
                    AntiPattern *ap = &func->patterns->anti_patterns[p];
                    fprintf(out, "    " ANSI_YELLOW "[!] %s" ANSI_RESET " (L%d): %s\n",
                            ap->name, ap->lineno, ap->description);
                    fprintf(out, "      Suggestion: %s\n", ap->suggestion);
                }
            }

            /* Show dead code */
            if (fr->dead_code && fr->dead_code->count > 0) {
                for (size_t d = 0; d < fr->dead_code->count; d++) {
                    DeadCodeItem *dc = &fr->dead_code->items[d];
                    fprintf(out, "    " ANSI_DIM "[dead] %s" ANSI_RESET " (L%d): %s\n",
                            dc->kind, dc->lineno, dc->description);
                }
            }
        }
        fprintf(out, "\n");
    }

    /* Summary */
    if (analysis_total_issues(report) > 0) {
        fprintf(out, ANSI_BOLD "Suggestions:" ANSI_RESET "\n");
        for (size_t f = 0; f < report->files_count; f++) {
            FileReport *fr = report->files[f];
            for (size_t i = 0; i < fr->functions_count; i++) {
                FunctionReport *func = fr->functions[i];
                if (func->patterns) {
                    for (size_t p = 0; p < func->patterns->count; p++) {
                        fprintf(out, "  - %s:%s (L%d): %s\n",
                                fr->filepath, func->name,
                                func->patterns->anti_patterns[p].lineno,
                                func->patterns->anti_patterns[p].suggestion);
                    }
                }
                if (func->complexity) {
                    for (size_t w = 0; w < func->complexity->warnings_count; w++) {
                        fprintf(out, "  - %s:%s: %s\n",
                                fr->filepath, func->name,
                                func->complexity->warnings[w]);
                    }
                }
            }
        }
    }

    if (output_path) fclose(out);
}