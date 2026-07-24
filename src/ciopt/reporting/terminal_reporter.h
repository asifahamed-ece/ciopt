#ifndef CIOPT_TERMINAL_REPORTER_H
#define CIOPT_TERMINAL_REPORTER_H

#include "report.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Render analysis report to terminal with ANSI colors.
 * If output_path is non-NULL, writes to file instead of stdout. */
void render_terminal(AnalysisReport *report, bool verbose, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_TERMINAL_REPORTER_H */