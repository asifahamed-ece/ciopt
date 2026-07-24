#ifndef CIOPT_HASHMAP_H
#define CIOPT_HASHMAP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Hash map — string-keyed, void* value storage.
 *
 * Uses open addressing with FNV-1a hashing and linear probing.
 * Keys are copied (strdup'd) on insert, freed on removal.
 * Values are stored as void* — caller manages value memory.
 *============================================================================*/

typedef struct HashMapEntry {
    char *key;          /* owned copy of key string */
    void *value;        /* pointer to value (caller-owned) */
    bool occupied;      /* slot is in use */
    bool tombstone;     /* slot was deleted (allows probing to skip) */
} HashMapEntry;

typedef struct HashMap {
    HashMapEntry *entries;
    size_t capacity;
    size_t count;       /* number of active entries (occupied, not tombstone) */
    size_t threshold;   /* resize when count >= threshold */
} HashMap;

/* Create hash map with initial capacity. Returns NULL on failure. */
HashMap *hashmap_create(size_t initial_capacity);

/* Destroy hash map. If value_free is non-NULL, calls it on each value. */
void hashmap_free(HashMap *m, void (*value_free)(void *));

/* Insert key-value pair. Overwrites existing key. Returns 0 on success, -1 on failure. */
int hashmap_put(HashMap *m, const char *key, void *value);

/* Look up key. Returns value pointer, or NULL if not found. */
void *hashmap_get(const HashMap *m, const char *key);

/* Check if key exists. */
bool hashmap_contains(const HashMap *m, const char *key);

/* Remove key. Returns 0 if found, -1 if not found.
 * If value_out is non-NULL, stores the value before removal. */
int hashmap_remove(HashMap *m, const char *key, void **value_out);

/* Clear all entries. If value_free is non-NULL, calls it on each value. */
void hashmap_clear(HashMap *m, void (*value_free)(void *));

/* Get number of entries. */
size_t hashmap_count(const HashMap *m);

/* Check if hash map is empty. */
bool hashmap_is_empty(const HashMap *m);

/* Check if resize is needed. Returns 0 on success, -1 on failure. */
int hashmap_resize(HashMap *m, size_t new_capacity);

/* Iterate over all entries. Returns true if more entries remain.
 *
 * Usage:
 *     size_t i = 0;
 *     const char *key;
 *     void *value;
 *     while (hashmap_iter(m, &i, &key, &value)) {
 *         printf("%s -> %p\n", key, value);
 *     }
 */
bool hashmap_iter(const HashMap *m, size_t *index, const char **key, void **value);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_HASHMAP_H */