#ifndef CIOPT_API_H
#define CIOPT_API_H

#include "config.h"
#include "reporting/report.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * CiOpt Public API
 *============================================================================*/

/* Analyze a C file or directory.
 * Returns AnalysisReport with complete results. Caller must free with analysis_report_free(). */
AnalysisReport *ciopt_analyze(const char *path, AnalysisConfig *config);

/* Analyze C source code from a string.
 * Useful for testing or analyzing code snippets.
 * filename is used for error messages (can be NULL). */
AnalysisReport *ciopt_analyze_source(const char *source, const char *filename,
                                      AnalysisConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_API_H */