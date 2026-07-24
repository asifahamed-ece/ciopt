#ifndef CIOPT_CONFIG_H
#define CIOPT_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Configuration types and enums for CiOpt analysis.
 *============================================================================*/

/* Severity levels for analysis findings */
typedef enum {
    SEVERITY_INFO = 0,
    SEVERITY_WARNING,
    SEVERITY_CRITICAL
} Severity;

/* Output formats */
typedef enum {
    FORMAT_TERMINAL = 0,
    FORMAT_HTML,
    FORMAT_JSON
} ReportFormat;

/* Known algorithmic complexity classes, ordered from best to worst */
typedef enum {
    COMPLEXITY_O_1 = 0,         /* O(1) */
    COMPLEXITY_O_LOG_N,         /* O(log n) */
    COMPLEXITY_O_N,             /* O(n) */
    COMPLEXITY_O_N_LOG_N,       /* O(n log n) */
    COMPLEXITY_O_N_SQUARED,     /* O(n²) */
    COMPLEXITY_O_N_CUBED,       /* O(n³) */
    COMPLEXITY_O_N_K,           /* O(nᵏ) polynomial, k > 3 */
    COMPLEXITY_O_2_N,           /* O(2ⁿ) */
    COMPLEXITY_O_N_FACTORIAL,   /* O(n!) */
    COMPLEXITY_UNKNOWN          /* Unknown */
} ComplexityClass;

/* Get string representation of a complexity class */
const char *complexity_class_to_string(ComplexityClass c);

/* Get rank for comparison (higher = worse) */
int complexity_class_rank(ComplexityClass c);

/* Compare two complexity classes */
int complexity_class_compare(ComplexityClass a, ComplexityClass b);

/* Analysis configuration */
typedef struct {
    ComplexityClass complexity_warning_threshold;   /* Flag as warning at this level */
    ComplexityClass complexity_critical_threshold;  /* Flag as critical at this level */
    int max_nesting_depth;                          /* Max nesting before warning */
    bool detect_dead_code;                          /* Enable dead code detection */
    bool detect_anti_patterns;                      /* Enable anti-pattern detection */
    bool detect_data_structure_issues;              /* Enable data structure analysis */
    bool detect_recursion_issues;                   /* Enable recursion analysis */
    ReportFormat report_format;                     /* Output format */
    char *output_path;                              /* Output file path (optional) */
    bool include_source;                            /* Include source snippets in reports */
    bool verbose;                                   /* Verbose output */
    size_t file_extensions_count;                   /* Number of file extensions */
    char **file_extensions;                         /* File extensions to analyze */
    size_t exclude_dirs_count;                      /* Number of excluded dirs */
    char **exclude_dirs;                            /* Directory names to exclude */
} AnalysisConfig;

/* Create default analysis configuration. Caller must free with config_free(). */
AnalysisConfig *config_create_default(void);

/* Free analysis configuration */
void config_free(AnalysisConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_CONFIG_H */