#include "string_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_INIT_CAP 64

StringBuilder *sb_create(void)
{
    return sb_create_with_capacity(SB_INIT_CAP);
}

StringBuilder *sb_create_with_capacity(size_t initial_capacity)
{
    StringBuilder *sb = (StringBuilder *)calloc(1, sizeof(StringBuilder));
    if (!sb) return NULL;

    if (initial_capacity < 16)
        initial_capacity = 16;

    sb->buffer = (char *)malloc(initial_capacity);
    if (!sb->buffer) {
        free(sb);
        return NULL;
    }

    sb->buffer[0] = '\0';
    sb->length = 0;
    sb->capacity = initial_capacity;
    return sb;
}

void sb_free(StringBuilder *sb, char **out_str)
{
    if (!sb) return;

    if (out_str) {
        *out_str = sb->buffer;
    } else {
        free(sb->buffer);
    }

    free(sb);
}

static int _ensure_capacity(StringBuilder *sb, size_t needed)
{
    if (sb->capacity >= needed)
        return 0;

    size_t new_cap = sb->capacity;
    while (new_cap < needed)
        new_cap *= 2;

    char *new_buf = (char *)realloc(sb->buffer, new_cap);
    if (!new_buf) return -1;

    sb->buffer = new_buf;
    sb->capacity = new_cap;
    return 0;
}

int sb_append(StringBuilder *sb, const char *str)
{
    if (!sb || !str) return -1;
    return sb_append_substr(sb, str, strlen(str));
}

int sb_append_char(StringBuilder *sb, char c)
{
    if (!sb) return -1;

    if (_ensure_capacity(sb, sb->length + 2) != 0)
        return -1;

    sb->buffer[sb->length++] = c;
    sb->buffer[sb->length] = '\0';
    return 0;
}

int sb_appendf(StringBuilder *sb, const char *fmt, ...)
{
    if (!sb || !fmt) return -1;

    va_list args;
    va_start(args, fmt);
    int ret = sb_vappendf(sb, fmt, args);
    va_end(args);
    return ret;
}

int sb_vappendf(StringBuilder *sb, const char *fmt, va_list args)
{
    if (!sb || !fmt) return -1;

    va_list args_copy;
    va_copy(args_copy, args);

    /* Try to format into the remaining space */
    int avail = (int)(sb->capacity - sb->length);
    int written = vsnprintf(sb->buffer + sb->length, (size_t)avail, fmt, args);

    if (written < 0) {
        va_end(args_copy);
        return -1;
    }

    if ((size_t)written >= (size_t)avail) {
        /* Need more space */
        size_t needed = sb->length + (size_t)written + 1;
        if (_ensure_capacity(sb, needed) != 0) {
            va_end(args_copy);
            return -1;
        }

        written = vsnprintf(sb->buffer + sb->length,
                           sb->capacity - sb->length, fmt, args_copy);
        if (written < 0) {
            va_end(args_copy);
            return -1;
        }
    }

    sb->length += (size_t)written;
    va_end(args_copy);
    return 0;
}

int sb_append_n(StringBuilder *sb, char c, size_t n)
{
    if (!sb) return -1;

    if (_ensure_capacity(sb, sb->length + n + 1) != 0)
        return -1;

    memset(sb->buffer + sb->length, c, n);
    sb->length += n;
    sb->buffer[sb->length] = '\0';
    return 0;
}

int sb_append_substr(StringBuilder *sb, const char *str, size_t len)
{
    if (!sb || !str) return -1;

    if (_ensure_capacity(sb, sb->length + len + 1) != 0)
        return -1;

    memcpy(sb->buffer + sb->length, str, len);
    sb->length += len;
    sb->buffer[sb->length] = '\0';
    return 0;
}

const char *sb_to_string(const StringBuilder *sb)
{
    return sb ? sb->buffer : NULL;
}

size_t sb_length(const StringBuilder *sb)
{
    return sb ? sb->length : 0;
}

void sb_clear(StringBuilder *sb)
{
    if (!sb) return;
    sb->buffer[0] = '\0';
    sb->length = 0;
}

int sb_reserve(StringBuilder *sb, size_t additional_len)
{
    if (!sb) return -1;
    return _ensure_capacity(sb, sb->length + additional_len + 1);
}