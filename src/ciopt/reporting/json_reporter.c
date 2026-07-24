#include "json_reporter.h"
#include "../utils/string_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _escape_json(StringBuilder *sb, const char *str)
{
    if (!str) {
        sb_append(sb, "null");
        return;
    }

    sb_append_char(sb, '"');
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  sb_append(sb, "\\\""); break;
            case '\\': sb_append(sb, "\\\\"); break;
            case '\n': sb_append(sb, "\\n"); break;
            case '\r': sb_append(sb, "\\r"); break;
            case '\t': sb_append(sb, "\\t"); break;
            default:   sb_append_char(sb, *p); break;
        }
    }
    sb_append_char(sb, '"');
}

char *render_json(AnalysisReport *report)
{
    if (!report) return NULL;

    StringBuilder *sb = sb_create();
    if (!sb) return NULL;

    sb_append(sb, "{\n");
    sb_appendf(sb, "  \"tool\": \"CiOpt\",\n");
    sb_appendf(sb, "  \"version\": \"%s\",\n", report->ciopt_version);
    sb_appendf(sb, "  \"timestamp\": \"%s\",\n", report->timestamp);
    sb_appendf(sb, "  \"analysis_duration_ms\": %.1f,\n", report->analysis_duration_ms);
    sb_appendf(sb, "  \"total_files\": %zu,\n", report->files_count);
    sb_appendf(sb, "  \"total_functions\": %d,\n", analysis_total_functions(report));
    sb_appendf(sb, "  \"worst_complexity\": \"%s\",\n",
               complexity_class_to_string(analysis_worst_complexity(report)));
    sb_appendf(sb, "  \"total_issues\": %d,\n", analysis_total_issues(report));
    sb_append(sb, "  \"files\": [\n");

    for (size_t f = 0; f < report->files_count; f++) {
        FileReport *fr = report->files[f];

        sb_append(sb, "    {\n");
        _escape_json(sb, fr->filepath);
        sb_appendf(sb, ": {\n");
        sb_appendf(sb, "      \"line_count\": %d,\n", fr->line_count);
        sb_appendf(sb, "      \"functions\": [\n");

        for (size_t i = 0; i < fr->functions_count; i++) {
            FunctionReport *func = fr->functions[i];

            sb_append(sb, "        {\n");
            _escape_json(sb, func->name);
            sb_append(sb, ": {\n");
            sb_appendf(sb, "          \"lineno\": %d,\n", func->lineno);
            sb_appendf(sb, "          \"end_lineno\": %d,\n", func->end_lineno);
            sb_appendf(sb, "          \"line_count\": %d,\n", func->line_count);

            if (func->complexity) {
                sb_appendf(sb, "          \"complexity\": \"%s\",\n",
                    complexity_class_to_string(func->complexity->estimated_complexity));
                sb_appendf(sb, "          \"confidence\": %.2f,\n",
                    func->complexity->confidence);
                sb_appendf(sb, "          \"explanations\": [\n");
                for (size_t e = 0; e < func->complexity->explanations_count; e++) {
                    ComplexityExplanation *exp = &func->complexity->explanations[e];
                    sb_append(sb, "            {\n");
                    sb_appendf(sb, "              \"source\": \"%s\",\n", exp->source);
                    sb_appendf(sb, "              \"complexity\": \"%s\",\n",
                        complexity_class_to_string(exp->complexity));
                    sb_appendf(sb, "              \"lineno\": %d,\n", exp->lineno);
                    sb_append(sb, "              \"description\": ");
                    _escape_json(sb, exp->description);
                    sb_append(sb, "\n            }");
                    if (e < func->complexity->explanations_count - 1)
                        sb_append(sb, ",");
                    sb_append(sb, "\n");
                }
                sb_append(sb, "          ],\n");

                /* Warnings */
                sb_append(sb, "          \"warnings\": [");
                for (size_t w = 0; w < func->complexity->warnings_count; w++) {
                    _escape_json(sb, func->complexity->warnings[w]);
                    if (w < func->complexity->warnings_count - 1)
                        sb_append(sb, ", ");
                }
                sb_append(sb, "],\n");
            }

            /* Severity */
            const char *sev_str = "info";
            if (func->severity == SEVERITY_CRITICAL) sev_str = "critical";
            else if (func->severity == SEVERITY_WARNING) sev_str = "warning";
            sb_appendf(sb, "          \"severity\": \"%s\"\n", sev_str);

            sb_append(sb, "        }}");
            if (i < fr->functions_count - 1)
                sb_append(sb, ",");
            sb_append(sb, "\n");
        }

        sb_append(sb, "      ]\n    }}");
        if (f < report->files_count - 1)
            sb_append(sb, ",");
        sb_append(sb, "\n");
    }

    sb_append(sb, "  ]\n}\n");

    char *result = NULL;
    sb_free(sb, &result);
    return result;
}

const char *save_json_report(AnalysisReport *report, const char *output_path)
{
    if (!report || !output_path) return NULL;

    char *json = render_json(report);
    if (!json) return NULL;

    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        free(json);
        return NULL;
    }

    fputs(json, fp);
    fclose(fp);
    free(json);

    return output_path;
}