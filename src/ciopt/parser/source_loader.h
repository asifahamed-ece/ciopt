#ifndef CIOPT_SOURCE_LOADER_H
#define CIOPT_SOURCE_LOADER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Source file loading and project scanning.
 *============================================================================*/

typedef struct {
    char *path;         /* full path to file */
    char *content;      /* file content (null-terminated) */
    size_t content_len; /* length of content */
    int line_count;     /* number of lines */
} SourceFile;

/* Create a SourceFile from file path. Returns NULL on failure. */
SourceFile *source_load(const char *path);

/* Create a SourceFile from a string (for API use). */
SourceFile *source_from_string(const char *content, const char *filename);

/* Free a SourceFile */
void source_free(SourceFile *sf);

/* Get a specific line (1-indexed). Returns NULL if out of range.
 * Caller must free the returned string. */
char *source_get_line(const SourceFile *sf, int lineno);

/* Scan a directory for C source files.
 * Returns a NULL-terminated array of SourceFile pointers.
 * Caller must free with source_scan_free(). */
SourceFile **source_scan_directory(const char *dir_path,
                                    const char *const *extensions,
                                    size_t ext_count
#ifdef _WIN32
                                    , const char *const *exclude_dirs,
                                    size_t exclude_count
#endif
                                    );

/* Free a scanned array of SourceFile pointers. */
void source_scan_free(SourceFile **files);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_SOURCE_LOADER_H */