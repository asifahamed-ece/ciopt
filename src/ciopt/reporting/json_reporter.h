#ifndef CIOPT_JSON_REPORTER_H
#define CIOPT_JSON_REPORTER_H

#include "report.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Render analysis report as a JSON string.
 * Caller must free the returned string. */
char *render_json(AnalysisReport *report);

/* Save JSON report to file. Returns the output path on success, NULL on failure. */
const char *save_json_report(AnalysisReport *report, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_JSON_REPORTER_H */