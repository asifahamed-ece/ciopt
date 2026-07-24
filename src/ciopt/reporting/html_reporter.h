#ifndef CIOPT_HTML_REPORTER_H
#define CIOPT_HTML_REPORTER_H

#include "report.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Save analysis report as a self-contained HTML file.
 * Returns the output path on success, NULL on failure. */
const char *save_html_report(AnalysisReport *report, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_HTML_REPORTER_H */