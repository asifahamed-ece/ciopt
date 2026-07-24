#include "html_reporter.h"
#include "../utils/string_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *save_html_report(AnalysisReport *report, const char *output_path)
{
    if (!report || !output_path) return NULL;

    FILE *fp = fopen(output_path, "w");
    if (!fp) return NULL;

    /* Determine output path for return */
    const char *result = output_path;

    /* HTML Header */
    fprintf(fp, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n");
    fprintf(fp, "<meta charset=\"UTF-8\">\n");
    fprintf(fp, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(fp, "<title>CiOpt Analysis Report</title>\n");
    fprintf(fp, "<style>\n");
    fprintf(fp, "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; ");
    fprintf(fp, "margin: 0; padding: 20px; background: #f5f5f5; color: #333; }\n");
    fprintf(fp, "  .container { max-width: 960px; margin: 0 auto; }\n");
    fprintf(fp, "  h1 { color: #1a73e8; border-bottom: 2px solid #1a73e8; padding-bottom: 10px; }\n");
    fprintf(fp, "  .summary { background: white; border-radius: 8px; padding: 20px; margin: 20px 0; ");
    fprintf(fp, "box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n");
    fprintf(fp, "  .summary-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); ");
    fprintf(fp, "gap: 15px; }\n");
    fprintf(fp, "  .stat { text-align: center; padding: 15px; background: #f8f9fa; border-radius: 6px; }\n");
    fprintf(fp, "  .stat-value { font-size: 24px; font-weight: bold; color: #1a73e8; }\n");
    fprintf(fp, "  .stat-label { font-size: 12px; color: #666; margin-top: 5px; }\n");
    fprintf(fp, "  .file-section { background: white; border-radius: 8px; padding: 20px; margin: 15px 0; ");
    fprintf(fp, "box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n");
    fprintf(fp, "  .file-header { font-size: 18px; font-weight: bold; color: #333; margin-bottom: 15px; }\n");
    fprintf(fp, "  .function { padding: 10px; margin: 8px 0; border-radius: 4px; border-left: 4px solid #ddd; }\n");
    fprintf(fp, "  .severity-critical { border-left-color: #d93025; background: #fce8e6; }\n");
    fprintf(fp, "  .severity-warning { border-left-color: #f9ab00; background: #fef7e0; }\n");
    fprintf(fp, "  .severity-info { border-left-color: #1a73e8; background: #e8f0fe; }\n");
    fprintf(fp, "  .func-name { font-weight: bold; font-family: 'Courier New', monospace; }\n");
    fprintf(fp, "  .complexity { display: inline-block; padding: 2px 8px; border-radius: 3px; ");
    fprintf(fp, "font-size: 12px; font-weight: bold; margin-left: 10px; }\n");
    fprintf(fp, "  .complexity-high { background: #d93025; color: white; }\n");
    fprintf(fp, "  .complexity-mid { background: #f9ab00; color: white; }\n");
    fprintf(fp, "  .complexity-low { background: #1a73e8; color: white; }\n");
    fprintf(fp, "  .issue { margin: 5px 0; padding: 5px 10px; background: #fff; border-radius: 3px; ");
    fprintf(fp, "font-size: 13px; }\n");
    fprintf(fp, "  .suggestion { color: #1a73e8; font-style: italic; }\n");
    fprintf(fp, "  footer { text-align: center; color: #999; font-size: 12px; margin-top: 30px; }\n");
    fprintf(fp, "</style>\n</head>\n<body>\n");
    fprintf(fp, "<div class=\"container\">\n");

    /* Title */
    fprintf(fp, "<h1>CiOpt Analysis Report</h1>\n");
    fprintf(fp, "<p>Generated: %s | Version: %s | Time: %.1f ms</p>\n",
            report->timestamp, report->ciopt_version, report->analysis_duration_ms);

    /* Summary */
    fprintf(fp, "<div class=\"summary\">\n");
    fprintf(fp, "<h2>Summary</h2>\n");
    fprintf(fp, "<div class=\"summary-grid\">\n");
    fprintf(fp, "  <div class=\"stat\"><div class=\"stat-value\">%d</div>");
    fprintf(fp, "<div class=\"stat-label\">Files</div></div>\n", (int)report->files_count);
    fprintf(fp, "  <div class=\"stat\"><div class=\"stat-value\">%d</div>");
    fprintf(fp, "<div class=\"stat-label\">Functions</div></div>\n", analysis_total_functions(report));
    fprintf(fp, "  <div class=\"stat\"><div class=\"stat-value\">%s</div>");
    fprintf(fp, "<div class=\"stat-label\">Worst Complexity</div></div>\n",
            complexity_class_to_string(analysis_worst_complexity(report)));
    fprintf(fp, "  <div class=\"stat\"><div class=\"stat-value\">%d</div>");
    fprintf(fp, "<div class=\"stat-label\">Issues</div></div>\n", analysis_total_issues(report));
    fprintf(fp, "</div></div>\n");

    /* File sections */
    for (size_t f = 0; f < report->files_count; f++) {
        FileReport *fr = report->files[f];
        fprintf(fp, "<div class=\"file-section\">\n");
        fprintf(fp, "<div class=\"file-header\">%s (%d lines)</div>\n",
                fr->filepath, fr->line_count);

        for (size_t i = 0; i < fr->functions_count; i++) {
            FunctionReport *func = fr->functions[i];
            const char *sev_class = "severity-info";
            const char *cpx_class = "complexity-low";
            if (func->severity == SEVERITY_CRITICAL) sev_class = "severity-critical";
            else if (func->severity == SEVERITY_WARNING) sev_class = "severity-warning";

            ComplexityClass cc = func->complexity ?
                func->complexity->estimated_complexity : COMPLEXITY_UNKNOWN;
            int rank = complexity_class_rank(cc);
            if (rank >= 4) cpx_class = "complexity-high";
            else if (rank >= 2) cpx_class = "complexity-mid";

            fprintf(fp, "<div class=\"function %s\">\n", sev_class);
            fprintf(fp, "  <span class=\"func-name\">%s</span>", func->name);
            fprintf(fp, "  <span class=\"complexity %s\">%s</span>\n",
                    cpx_class, complexity_class_to_string(cc));
            fprintf(fp, "  <span style=\"font-size:12px;color:#666;\">");
            fprintf(fp, "(L%d-%d, %d lines)</span>\n",
                    func->lineno, func->end_lineno, func->line_count);

            /* Anti-patterns */
            if (func->patterns && func->patterns->count > 0) {
                fprintf(fp, "  <div style=\"margin-top:8px;\">\n");
                for (size_t p = 0; p < func->patterns->count; p++) {
                    AntiPattern *ap = &func->patterns->anti_patterns[p];
                    fprintf(fp, "    <div class=\"issue\">");
                    fprintf(fp, "<strong>⚠ %s</strong> (L%d): %s<br>",
                            ap->name, ap->lineno, ap->description);
                    fprintf(fp, "    <span class=\"suggestion\">💡 %s</span>", ap->suggestion);
                    fprintf(fp, "</div>\n");
                }
                fprintf(fp, "  </div>\n");
            }

            /* Warnings from complexity */
            if (func->complexity && func->complexity->warnings_count > 0) {
                for (size_t w = 0; w < func->complexity->warnings_count; w++) {
                    fprintf(fp, "  <div class=\"issue\">⚠ %s</div>\n",
                            func->complexity->warnings[w]);
                }
            }

            fprintf(fp, "</div>\n");
        }

        fprintf(fp, "</div>\n");
    }

    /* Footer */
    fprintf(fp, "<footer>CiOpt v%s — Developed by Asif Ahamed</footer>\n", report->ciopt_version);
    fprintf(fp, "</div>\n</body>\n</html>\n");

    fclose(fp);
    return result;
}