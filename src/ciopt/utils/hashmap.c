#include "hashmap.h"
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

/* FNV-1a hash (32-bit) */
static unsigned long _hash(const char *key, size_t capacity)
{
    unsigned long hash = 2166136261UL;
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= 16777619UL;
    }
    return hash % capacity;
}

/* Default initial capacity */
#define HASHMAP_INIT_CAP 32
#define HASHMAP_LOAD_FACTOR 0.75

static int _resize(HashMap *m, size_t new_capacity)
{
    HashMapEntry *old = m->entries;
    size_t old_cap = m->capacity;

    m->entries = (HashMapEntry *)calloc(new_capacity, sizeof(HashMapEntry));
    if (!m->entries) {
        m->entries = old;
        return -1;
    }

    m->capacity = new_capacity;
    m->threshold = (size_t)(new_capacity * HASHMAP_LOAD_FACTOR);
    m->count = 0;

    /* Rehash old entries */
    for (size_t i = 0; i < old_cap; i++) {
        if (old[i].occupied && !old[i].tombstone) {
            if (hashmap_put(m, old[i].key, old[i].value) != 0) {
                /* Partial failure — free what we can */
                for (size_t j = 0; j < i; j++) {
                    if (old[j].occupied && !old[j].tombstone) {
                        free(old[j].key);
                    }
                }
                free(old);
                return -1;
            }
            free(old[i].key); /* key was strdup'd by _resize caller */
            old[i].key = NULL;
        }
    }

    free(old);
    return 0;
}

HashMap *hashmap_create(size_t initial_capacity)
{
    HashMap *m = (HashMap *)calloc(1, sizeof(HashMap));
    if (!m) return NULL;

    if (initial_capacity < HASHMAP_INIT_CAP)
        initial_capacity = HASHMAP_INIT_CAP;

    m->entries = (HashMapEntry *)calloc(initial_capacity, sizeof(HashMapEntry));
    if (!m->entries) {
        free(m);
        return NULL;
    }

    m->capacity = initial_capacity;
    m->count = 0;
    m->threshold = (size_t)(initial_capacity * HASHMAP_LOAD_FACTOR);
    return m;
}

void hashmap_free(HashMap *m, void (*value_free)(void *))
{
    if (!m) return;

    for (size_t i = 0; i < m->capacity; i++) {
        if (m->entries[i].occupied && !m->entries[i].tombstone) {
            free(m->entries[i].key);
            if (value_free && m->entries[i].value)
                value_free(m->entries[i].value);
        }
    }

    free(m->entries);
    free(m);
}

int hashmap_put(HashMap *m, const char *key, void *value)
{
    if (!m || !key) return -1;

    /* Resize if needed */
    if (m->count >= m->threshold) {
        if (_resize(m, m->capacity * 2) != 0)
            return -1;
    }

    unsigned long idx = _hash(key, m->capacity);

    /* Linear probing */
    for (size_t i = 0; i < m->capacity; i++) {
        size_t probe = (idx + i) % m->capacity;

        if (!m->entries[probe].occupied || m->entries[probe].tombstone) {
            /* Empty slot — insert */
            m->entries[probe].key = strdup(key);
            if (!m->entries[probe].key) return -1;
            m->entries[probe].value = value;
            m->entries[probe].occupied = true;
            m->entries[probe].tombstone = false;
            m->count++;
            return 0;
        }

        if (m->entries[probe].occupied &&
            strcmp(m->entries[probe].key, key) == 0) {
            /* Key exists — overwrite */
            m->entries[probe].value = value;
            return 0;
        }
    }

    return -1; /* Should not happen if capacity > 0 */
}

void *hashmap_get(const HashMap *m, const char *key)
{
    if (!m || !key) return NULL;

    unsigned long idx = _hash(key, m->capacity);

    for (size_t i = 0; i < m->capacity; i++) {
        size_t probe = (idx + i) % m->capacity;

        if (!m->entries[probe].occupied)
            return NULL; /* empty slot means key not present */

        if (m->entries[probe].occupied &&
            !m->entries[probe].tombstone &&
            strcmp(m->entries[probe].key, key) == 0) {
            return m->entries[probe].value;
        }
    }

    return NULL;
}

bool hashmap_contains(const HashMap *m, const char *key)
{
    return hashmap_get(m, key) != NULL;
}

int hashmap_remove(HashMap *m, const char *key, void **value_out)
{
    if (!m || !key) return -1;

    unsigned long idx = _hash(key, m->capacity);

    for (size_t i = 0; i < m->capacity; i++) {
        size_t probe = (idx + i) % m->capacity;

        if (!m->entries[probe].occupied)
            return -1; /* not found */

        if (m->entries[probe].occupied &&
            !m->entries[probe].tombstone &&
            strcmp(m->entries[probe].key, key) == 0) {
            /* Found */
            if (value_out)
                *value_out = m->entries[probe].value;

            free(m->entries[probe].key);
            m->entries[probe].key = NULL;
            m->entries[probe].value = NULL;
            m->entries[probe].tombstone = true;
            m->count--;
            return 0;
        }
    }

    return -1;
}

void hashmap_clear(HashMap *m, void (*value_free)(void *))
{
    if (!m) return;

    for (size_t i = 0; i < m->capacity; i++) {
        if (m->entries[i].occupied && !m->entries[i].tombstone) {
            free(m->entries[i].key);
            if (value_free && m->entries[i].value)
                value_free(m->entries[i].value);
        }
        m->entries[i].occupied = false;
        m->entries[i].tombstone = false;
        m->entries[i].key = NULL;
        m->entries[i].value = NULL;
    }

    m->count = 0;
}

size_t hashmap_count(const HashMap *m)
{
    return m ? m->count : 0;
}

bool hashmap_is_empty(const HashMap *m)
{
    return m ? m->count == 0 : true;
}

bool hashmap_iter(const HashMap *m, size_t *index, const char **key, void **value)
{
    if (!m || !index) return false;

    while (*index < m->capacity) {
        if (m->entries[*index].occupied && !m->entries[*index].tombstone) {
            if (key) *key = m->entries[*index].key;
            if (value) *value = m->entries[*index].value;
            (*index)++;
            return true;
        }
        (*index)++;
    }

    return false;
}