#ifndef CIOPT_VECTOR_H
#define CIOPT_VECTOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Dynamic array (vector) — generic, type-safe via macros.
 *
 * Internal storage: void** array with doubling growth strategy.
 * All functions return -1 on failure, 0 on success where applicable.
 *============================================================================*/

typedef struct Vector {
    void **data;       /* array of void* elements */
    size_t size;       /* number of elements */
    size_t capacity;   /* allocated capacity */
    size_t element_size; /* size of each element (for copy) */
} Vector;

/* Create a new vector with given element size and initial capacity.
 * Returns NULL on allocation failure. */
Vector *vector_create(size_t element_size, size_t initial_capacity);

/* Destroy vector and optionally free elements via callback.
 * If free_func is NULL, only the container is freed (not the elements). */
void vector_free(Vector *v, void (*free_func)(void *));

/* Append an element (copied from src). Returns 0 on success, -1 on failure. */
int vector_push(Vector *v, const void *src);

/* Get element at index. Returns NULL if out of bounds. */
void *vector_get(const Vector *v, size_t index);

/* Set element at index (copies from src). Returns 0 on success, -1 on failure. */
int vector_set(Vector *v, size_t index, const void *src);

/* Remove last element and copy to out (if out != NULL). Returns 0 on success, -1 if empty. */
int vector_pop(Vector *v, void *out);

/* Remove element at index, shifting remaining elements left. Returns 0 on success. */
int vector_remove(Vector *v, size_t index);

/* Clear all elements (does not free elements unless free_func provided). */
void vector_clear(Vector *v, void (*free_func)(void *));

/* Reserve capacity for at least n elements. Returns 0 on success. */
int vector_reserve(Vector *v, size_t capacity);

/* Shrink capacity to fit size. Returns 0 on success. */
int vector_shrink_to_fit(Vector *v);

/* Get pointer to raw data array. */
void **vector_data(Vector *v);

/* Get number of elements. */
size_t vector_size(const Vector *v);

/* Get current capacity. */
size_t vector_capacity(const Vector *v);

/* Check if vector is empty. */
bool vector_is_empty(const Vector *v);

/* Sort vector using comparison function. */
void vector_sort(Vector *v, int (*cmp)(const void *, const void *));

/* Find first index of element matching key using comparison function.
 * Returns index or (size_t)-1 if not found. */
size_t vector_find(const Vector *v, const void *key,
                   int (*cmp)(const void *, const void *));

/* Iterate over vector. Usage:
 *    VECTOR_FOREACH(v, it) { it is a (T*) pointer to the element; }
 */
#define VECTOR_FOREACH(v, it) \
    for (size_t _i_ = 0; _i_ < (v)->size && ((it) = (void *)((v)->data[_i_])); ++_i_)

/* Type-safe push macro. Usage:
 *    int x = 42;
 *    vector_push(v, &x);  // copies sizeof(int) bytes
 */
#define VECTOR_PUSH(v, elem) \
    do { \
        __typeof__(elem) _tmp_ = (elem); \
        vector_push((v), &_tmp_); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_VECTOR_H */