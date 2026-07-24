#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "ciopt/api.h"
#include "ciopt/config.h"
#include "ciopt/reporting/terminal_reporter.h"
#include "ciopt/reporting/html_reporter.h"
#include "ciopt/reporting/json_reporter.h"
#include "ciopt/reporting/report.h"

static void print_usage(const char *prog)
{
    fprintf(stderr, "CiOpt - C Code Complexity Analysis Engine v0.1.0\n");
    fprintf(stderr, "Developed by Asif Ahamed\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s analyze <path> [options]\n", prog);
    fprintf(stderr, "  %s version\n", prog);
    fprintf(stderr, "  %s --help\n\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --format, -f    terminal|html|json   Output format (default: terminal)\n");
    fprintf(stderr, "  --output, -o    FILE                 Output file path\n");
    fprintf(stderr, "  --verbose, -v                        Show detailed explanations\n");
    fprintf(stderr, "  --threshold     O(n)|O(n^2)|O(n^3)    Complexity threshold (default: O(n^2))\n");
}

static void print_version(void)
{
    printf("CiOpt v0.1.0\n");
    printf("C Code Complexity Analysis Engine\n");
    printf("Developed by Asif Ahamed\n");
    printf("Built: %s %s\n", __DATE__, __TIME__);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "analyze") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Missing path argument\n");
            print_usage(argv[0]);
            return 1;
        }

        const char *path = argv[2];
        ReportFormat format = FORMAT_TERMINAL;
        const char *output_path = NULL;
        bool verbose = false;
        const char *threshold_str = "O(n^2)";

        /* Parse options */
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--format") == 0 || strcmp(argv[i], "-f") == 0) {
                if (i + 1 < argc) {
                    i++;
                    if (strcmp(argv[i], "html") == 0) format = FORMAT_HTML;
                    else if (strcmp(argv[i], "json") == 0) format = FORMAT_JSON;
                    else format = FORMAT_TERMINAL;
                }
            } else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) output_path = argv[++i];
            } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
                verbose = true;
            } else if (strcmp(argv[i], "--threshold") == 0) {
                if (i + 1 < argc) threshold_str = argv[++i];
            }
        }

        /* Create config */
        AnalysisConfig *config = config_create_default();
        if (!config) {
            fprintf(stderr, "Error: Failed to create configuration\n");
            return 1;
        }

        config->verbose = verbose;
        config->report_format = format;
        if (output_path) config->output_path = strdup(output_path);

        /* Set threshold */
        if (strcmp(threshold_str, "O(n)") == 0)
            config->complexity_warning_threshold = COMPLEXITY_O_N;
        else if (strcmp(threshold_str, "O(n^3)") == 0)
            config->complexity_warning_threshold = COMPLEXITY_O_N_CUBED;
        else
            config->complexity_warning_threshold = COMPLEXITY_O_N_SQUARED;

        /* Run analysis */
        AnalysisReport *report = ciopt_analyze(path, config);
        if (!report) {
            fprintf(stderr, "Error: Analysis failed\n");
            config_free(config);
            return 1;
        }

        /* Output */
        if (format == FORMAT_TERMINAL) {
            render_terminal(report, verbose, output_path);
        } else if (format == FORMAT_HTML) {
            const char *out = output_path ? output_path : "ciopt_report.html";
            const char *saved = save_html_report(report, out);
            if (saved)
                printf("HTML report saved to: %s\n", saved);
            else
                fprintf(stderr, "Error: Failed to save HTML report\n");
        } else if (format == FORMAT_JSON) {
            if (output_path) {
                const char *saved = save_json_report(report, output_path);
                if (saved)
                    printf("JSON report saved to: %s\n", saved);
                else
                    fprintf(stderr, "Error: Failed to save JSON report\n");
            } else {
                char *json = render_json(report);
                if (json) {
                    printf("%s\n", json);
                    free(json);
                }
            }
        }

        /* Determine exit code */
        int critical_count = 0;
        for (size_t f = 0; f < report->files_count; f++) {
            for (size_t i = 0; i < report->files[f]->functions_count; i++) {
                if (report->files[f]->functions[i]->severity == SEVERITY_CRITICAL)
                    critical_count++;
            }
        }

        analysis_report_free(report);
        config_free(config);

        return critical_count > 0 ? 1 : 0;
    }

    fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
    print_usage(argv[0]);
    return 1;
}