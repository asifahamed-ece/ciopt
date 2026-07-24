#include "vector.h"
#include <stdlib.h>
#include <string.h>

Vector *vector_create(size_t element_size, size_t initial_capacity)
{
    Vector *v = (Vector *)calloc(1, sizeof(Vector));
    if (!v) return NULL;

    if (initial_capacity == 0)
        initial_capacity = 8;

    v->data = (void **)calloc(initial_capacity, sizeof(void *));
    if (!v->data) {
        free(v);
        return NULL;
    }

    v->size = 0;
    v->capacity = initial_capacity;
    v->element_size = element_size;
    return v;
}

void vector_free(Vector *v, void (*free_func)(void *))
{
    if (!v) return;

    if (free_func) {
        for (size_t i = 0; i < v->size; i++) {
            if (v->data[i])
                free_func(v->data[i]);
        }
    }

    free(v->data);
    free(v);
}

int vector_push(Vector *v, const void *src)
{
    if (!v || !src) return -1;

    if (v->size >= v->capacity) {
        size_t new_cap = v->capacity * 2;
        void **new_data = (void **)realloc(v->data, new_cap * sizeof(void *));
        if (!new_data) return -1;
        v->data = new_data;
        v->capacity = new_cap;
    }

    v->data[v->size] = malloc(v->element_size);
    if (!v->data[v->size]) return -1;

    memcpy(v->data[v->size], src, v->element_size);
    v->size++;
    return 0;
}

void *vector_get(const Vector *v, size_t index)
{
    if (!v || index >= v->size) return NULL;
    return v->data[index];
}

int vector_set(Vector *v, size_t index, const void *src)
{
    if (!v || !src || index >= v->size) return -1;
    memcpy(v->data[index], src, v->element_size);
    return 0;
}

int vector_pop(Vector *v, void *out)
{
    if (!v || v->size == 0) return -1;

    v->size--;
    if (out)
        memcpy(out, v->data[v->size], v->element_size);

    free(v->data[v->size]);
    v->data[v->size] = NULL;
    return 0;
}

int vector_remove(Vector *v, size_t index)
{
    if (!v || index >= v->size) return -1;

    free(v->data[index]);

    for (size_t i = index; i < v->size - 1; i++)
        v->data[i] = v->data[i + 1];

    v->size--;
    v->data[v->size] = NULL;
    return 0;
}

void vector_clear(Vector *v, void (*free_func)(void *))
{
    if (!v) return;

    if (free_func) {
        for (size_t i = 0; i < v->size; i++) {
            if (v->data[i])
                free_func(v->data[i]);
        }
    }

    v->size = 0;
}

int vector_reserve(Vector *v, size_t capacity)
{
    if (!v || capacity <= v->capacity) return 0;

    void **new_data = (void **)realloc(v->data, capacity * sizeof(void *));
    if (!new_data) return -1;

    v->data = new_data;
    v->capacity = capacity;
    return 0;
}

int vector_shrink_to_fit(Vector *v)
{
    if (!v || v->size == v->capacity) return 0;

    if (v->size == 0) {
        free(v->data);
        v->data = NULL;
        v->capacity = 0;
        return 0;
    }

    void **new_data = (void **)realloc(v->data, v->size * sizeof(void *));
    if (!new_data) return -1;

    v->data = new_data;
    v->capacity = v->size;
    return 0;
}

void **vector_data(Vector *v)
{
    if (!v) return NULL;
    return v->data;
}

size_t vector_size(const Vector *v)
{
    return v ? v->size : 0;
}

size_t vector_capacity(const Vector *v)
{
    return v ? v->capacity : 0;
}

bool vector_is_empty(const Vector *v)
{
    return v ? v->size == 0 : true;
}

void vector_sort(Vector *v, int (*cmp)(const void *, const void *))
{
    if (!v || !cmp || v->size < 2) return;
    qsort(v->data, v->size, sizeof(void *), cmp);
}

size_t vector_find(const Vector *v, const void *key,
                   int (*cmp)(const void *, const void *))
{
    if (!v || !key || !cmp) return (size_t)-1;

    for (size_t i = 0; i < v->size; i++) {
        if (cmp(&v->data[i], key) == 0)
            return i;
    }

    return (size_t)-1;
}