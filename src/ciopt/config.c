#include "config.h"
#include <stdlib.h>
#include <string.h>

const char *complexity_class_to_string(ComplexityClass c)
{
    switch (c) {
        case COMPLEXITY_O_1:           return "O(1)";
        case COMPLEXITY_O_LOG_N:       return "O(log n)";
        case COMPLEXITY_O_N:           return "O(n)";
        case COMPLEXITY_O_N_LOG_N:     return "O(n log n)";
        case COMPLEXITY_O_N_SQUARED:   return "O(n^2)";
        case COMPLEXITY_O_N_CUBED:     return "O(n^3)";
        case COMPLEXITY_O_N_K:         return "O(n^k)";
        case COMPLEXITY_O_2_N:         return "O(2^n)";
        case COMPLEXITY_O_N_FACTORIAL: return "O(n!)";
        case COMPLEXITY_UNKNOWN:       return "Unknown";
        default:                       return "?";
    }
}

int complexity_class_rank(ComplexityClass c)
{
    switch (c) {
        case COMPLEXITY_O_1:           return 0;
        case COMPLEXITY_O_LOG_N:       return 1;
        case COMPLEXITY_O_N:           return 2;
        case COMPLEXITY_O_N_LOG_N:     return 3;
        case COMPLEXITY_O_N_SQUARED:   return 4;
        case COMPLEXITY_O_N_CUBED:     return 5;
        case COMPLEXITY_O_N_K:         return 6;
        case COMPLEXITY_O_2_N:         return 7;
        case COMPLEXITY_O_N_FACTORIAL: return 8;
        case COMPLEXITY_UNKNOWN:       return 9;
        default:                       return 9;
    }
}

int complexity_class_compare(ComplexityClass a, ComplexityClass b)
{
    int ra = complexity_class_rank(a);
    int rb = complexity_class_rank(b);
    if (ra < rb) return -1;
    if (ra > rb) return 1;
    return 0;
}

/* Default excluded directories */
static const char *DEFAULT_EXCLUDE_DIRS[] = {
    "__pycache__", ".git", ".venv", "venv", "node_modules",
    ".tox", ".eggs", "dist", "build", ".mypy_cache",
    ".svn", ".hg", "CMakeFiles", ".idea", ".vscode"
};
static const size_t DEFAULT_EXCLUDE_COUNT = 15;

/* Default file extensions */
static const char *DEFAULT_EXTENSIONS[] = { ".c", ".h" };
static const size_t DEFAULT_EXT_COUNT = 2;

AnalysisConfig *config_create_default(void)
{
    AnalysisConfig *config = (AnalysisConfig *)calloc(1, sizeof(AnalysisConfig));
    if (!config) return NULL;

    config->complexity_warning_threshold = COMPLEXITY_O_N_SQUARED;
    config->complexity_critical_threshold = COMPLEXITY_O_N_CUBED;
    config->max_nesting_depth = 3;
    config->detect_dead_code = true;
    config->detect_anti_patterns = true;
    config->detect_data_structure_issues = true;
    config->detect_recursion_issues = true;
    config->report_format = FORMAT_TERMINAL;
    config->output_path = NULL;
    config->include_source = true;
    config->verbose = false;

    /* Copy default extensions */
    config->file_extensions_count = DEFAULT_EXT_COUNT;
    config->file_extensions = (char **)calloc(DEFAULT_EXT_COUNT, sizeof(char *));
    if (config->file_extensions) {
        for (size_t i = 0; i < DEFAULT_EXT_COUNT; i++) {
            config->file_extensions[i] = strdup(DEFAULT_EXTENSIONS[i]);
        }
    }

    /* Copy default exclude dirs */
    config->exclude_dirs_count = DEFAULT_EXCLUDE_COUNT;
    config->exclude_dirs = (char **)calloc(DEFAULT_EXCLUDE_COUNT, sizeof(char *));
    if (config->exclude_dirs) {
        for (size_t i = 0; i < DEFAULT_EXCLUDE_COUNT; i++) {
            config->exclude_dirs[i] = strdup(DEFAULT_EXCLUDE_DIRS[i]);
        }
    }

    return config;
}

void config_free(AnalysisConfig *config)
{
    if (!config) return;

    free(config->output_path);

    if (config->file_extensions) {
        for (size_t i = 0; i < config->file_extensions_count; i++)
            free(config->file_extensions[i]);
        free(config->file_extensions);
    }

    if (config->exclude_dirs) {
        for (size_t i = 0; i < config->exclude_dirs_count; i++)
            free(config->exclude_dirs[i]);
        free(config->exclude_dirs);
    }

    free(config);
}