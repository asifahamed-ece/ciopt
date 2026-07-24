#ifndef CIOPT_STRING_BUILDER_H
#define CIOPT_STRING_BUILDER_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * String builder — efficient string construction.
 *
 * Avoids O(n²) strcat by maintaining a dynamic buffer.
 * All operations append to the end.
 *============================================================================*/

typedef struct StringBuilder {
    char *buffer;
    size_t length;      /* current string length (not including null terminator) */
    size_t capacity;    /* allocated capacity (including null terminator) */
} StringBuilder;

/* Create a new string builder. Returns NULL on failure. */
StringBuilder *sb_create(void);

/* Create with initial capacity. Returns NULL on failure. */
StringBuilder *sb_create_with_capacity(size_t initial_capacity);

/* Free string builder. If out_str is non-NULL, transfers ownership of the
 * internal buffer to the caller (caller must free it). */
void sb_free(StringBuilder *sb, char **out_str);

/* Append a string. Returns 0 on success, -1 on failure. */
int sb_append(StringBuilder *sb, const char *str);

/* Append a single character. Returns 0 on success, -1 on failure. */
int sb_append_char(StringBuilder *sb, char c);

/* Append formatted string (printf-style). Returns 0 on success, -1 on failure. */
int sb_appendf(StringBuilder *sb, const char *fmt, ...);

/* Append formatted string (va_list version). */
int sb_vappendf(StringBuilder *sb, const char *fmt, va_list args);

/* Append n copies of a character. Returns 0 on success, -1 on failure. */
int sb_append_n(StringBuilder *sb, char c, size_t n);

/* Append a substring (len bytes). Returns 0 on success, -1 on failure. */
int sb_append_substr(StringBuilder *sb, const char *str, size_t len);

/* Get current string (null-terminated). */
const char *sb_to_string(const StringBuilder *sb);

/* Get current length. */
size_t sb_length(const StringBuilder *sb);

/* Clear the builder (reset length to 0). */
void sb_clear(StringBuilder *sb);

/* Reserve capacity for at least additional_len more characters. */
int sb_reserve(StringBuilder *sb, size_t additional_len);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_STRING_BUILDER_H */