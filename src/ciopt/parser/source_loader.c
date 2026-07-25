#include "source_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

static bool _has_extension(const char *filename, const char *const *extensions, size_t ext_count)
{
    const char *dot = strrchr(filename, '.');
    if (!dot) return false;

    for (size_t i = 0; i < ext_count; i++) {
        if (strcmp(dot, extensions[i]) == 0)
            return true;
    }
    return false;
}

static bool _is_excluded(const char *dirname, const char *const *exclude_dirs, size_t exclude_count)
{
    for (size_t i = 0; i < exclude_count; i++) {
        if (strcmp(dirname, exclude_dirs[i]) == 0)
            return true;
    }
    return false;
}

static inline void _unused_is_excluded_silence(void) { (void)_is_excluded; }

SourceFile *source_load(const char *path)
{
    if (!path) return NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    /* Get file size by seeking to end */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size < 0) {
        fclose(fp);
        return NULL;
    }

    SourceFile *sf = (SourceFile *)calloc(1, sizeof(SourceFile));
    if (!sf) {
        fclose(fp);
        return NULL;
    }

    sf->path = strdup(path);
    sf->content_len = (size_t)file_size;
    sf->content = (char *)malloc(sf->content_len + 1);
    if (!sf->content) {
        free(sf->path);
        free(sf);
        fclose(fp);
        return NULL;
    }

    size_t bytes_read = fread(sf->content, 1, sf->content_len, fp);
    fclose(fp);

    sf->content[bytes_read] = '\0';
    sf->content_len = bytes_read;

    /* Count lines */
    sf->line_count = 1;
    for (size_t i = 0; i < sf->content_len; i++) {
        if (sf->content[i] == '\n')
            sf->line_count++;
    }

    return sf;
}

SourceFile *source_from_string(const char *content, const char *filename)
{
    if (!content) return NULL;

    SourceFile *sf = (SourceFile *)calloc(1, sizeof(SourceFile));
    if (!sf) return NULL;

    sf->path = strdup(filename ? filename : "<string>");
    sf->content_len = strlen(content);
    sf->content = strdup(content);

    /* Count lines */
    sf->line_count = 1;
    for (size_t i = 0; i < sf->content_len; i++) {
        if (sf->content[i] == '\n')
            sf->line_count++;
    }

    return sf;
}

void source_free(SourceFile *sf)
{
    if (!sf) return;
    free(sf->path);
    free(sf->content);
    free(sf);
}

char *source_get_line(const SourceFile *sf, int lineno)
{
    if (!sf || lineno < 1 || lineno > sf->line_count) return NULL;

    const char *p = sf->content;
    int current = 1;

    while (current < lineno && *p) {
        if (*p == '\n') current++;
        p++;
    }

    if (!*p) return NULL;

    /* Find end of line */
    const char *start = p;
    while (*p && *p != '\n') p++;

    size_t len = (size_t)(p - start);
    char *line = (char *)malloc(len + 1);
    if (!line) return NULL;

    memcpy(line, start, len);
    line[len] = '\0';
    return line;
}

/* Cross-platform file listing for directory scanning.
 * On Windows, uses FindFirstFile/FindNextFile.
 * On POSIX, uses opendir/readdir. */

#ifdef _WIN32

SourceFile **source_scan_directory(const char *dir_path,
                                    const char *const *extensions,
                                    size_t ext_count,
                                    const char *const *exclude_dirs,
                                    size_t exclude_count)
{
    (void)exclude_dirs;
    (void)exclude_count;

    if (!dir_path) return NULL;

    char search_path[1024];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(search_path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) return NULL;

    /* First pass: count files */
    size_t count = 0;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (_has_extension(ffd.cFileName, extensions, ext_count))
                count++;
        }
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);

    /* Allocate array */
    SourceFile **files = (SourceFile **)calloc(count + 1, sizeof(SourceFile *));
    if (!files) return NULL;

    /* Second pass: load files */
    hFind = FindFirstFile(search_path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        free(files);
        return NULL;
    }

    size_t idx = 0;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && idx < count) {
            if (_has_extension(ffd.cFileName, extensions, ext_count)) {
                size_t path_len = strlen(dir_path) + 1 + strlen(ffd.cFileName) + 1;
                char *full_path = (char *)malloc(path_len);
                if (full_path) {
                    snprintf(full_path, path_len, "%s\\%s", dir_path, ffd.cFileName);
                    files[idx] = source_load(full_path);
                    free(full_path);
                    if (files[idx])
                        idx++;
                }
            }
        }
    } while (FindNextFile(hFind, &ffd) != 0);
    FindClose(hFind);

    files[idx] = NULL;
    return files;
}

#else /* POSIX */

SourceFile **source_scan_directory(const char *dir_path,
                                    const char *const *extensions,
                                    size_t ext_count)
{
    if (!dir_path) return NULL;

    size_t count = 0;
    DIR *dir = opendir(dir_path);
    if (!dir) return NULL;

    struct dirent *entry;
    struct stat st;

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (_has_extension(entry->d_name, extensions, ext_count))
                count++;
        }
    }
    closedir(dir);

    SourceFile **files = (SourceFile **)calloc(count + 1, sizeof(SourceFile *));
    if (!files) return NULL;

    dir = opendir(dir_path);
    if (!dir) {
        free(files);
        return NULL;
    }

    size_t idx = 0;
    while ((entry = readdir(dir)) != NULL && idx < count) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (_has_extension(entry->d_name, extensions, ext_count)) {
                files[idx] = source_load(full_path);
                if (files[idx])
                    idx++;
            }
        }
    }
    closedir(dir);

    files[idx] = NULL;
    return files;
}

#endif

void source_scan_free(SourceFile **files)
{
    if (!files) return;
    for (size_t i = 0; files[i] != NULL; i++)
        source_free(files[i]);
    free(files);
}