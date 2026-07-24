#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ciopt/utils/vector.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        printf("FAIL: %s\n", name); \
        tests_failed++; \
    } else { \
        printf("PASS: %s\n", name); \
        tests_passed++; \
    } \
} while(0)

int main(void)
{
    printf("=== CiOpt Vector Tests ===\n\n");

    /* Test create */
    Vector *v = vector_create(sizeof(int), 4);
    TEST("create returns non-NULL", v != NULL);
    TEST("initial size is 0", vector_size(v) == 0);
    TEST("initial capacity >= 4", vector_capacity(v) >= 4);
    TEST("is_empty returns true", vector_is_empty(v));

    /* Test push */
    int val = 42;
    TEST("push returns 0", vector_push(v, &val) == 0);
    TEST("size is 1 after push", vector_size(v) == 1);
    TEST("is_empty returns false", !vector_is_empty(v));

    /* Test get */
    int *got = (int *)vector_get(v, 0);
    TEST("get returns correct value", got && *got == 42);

    /* Test multiple pushes */
    for (int i = 0; i < 100; i++) {
        vector_push(v, &i);
    }
    TEST("size is 101 after 101 pushes", vector_size(v) == 101);

    /* Test get all values */
    int all_ok = 1;
    for (size_t i = 0; i < vector_size(v); i++) {
        int *p = (int *)vector_get(v, i);
        if (i == 0) { if (*p != 42) all_ok = 0; }
        else { if (*p != (int)(i - 1)) all_ok = 0; }
    }
    TEST("all values correct", all_ok);

    /* Test pop */
    int popped = 0;
    TEST("pop returns 0", vector_pop(v, &popped) == 0);
    TEST("popped value is 99", popped == 99);
    TEST("size is 100 after pop", vector_size(v) == 100);

    /* Test set */
    int new_val = 999;
    TEST("set returns 0", vector_set(v, 0, &new_val) == 0);
    got = (int *)vector_get(v, 0);
    TEST("set updates value", got && *got == 999);

    /* Test remove */
    TEST("remove returns 0", vector_remove(v, 0) == 0);
    TEST("size is 99 after remove", vector_size(v) == 99);
    got = (int *)vector_get(v, 0);
    TEST("first element shifted after remove", got && *got == 0);

    /* Test clear */
    vector_clear(v, NULL);
    TEST("size is 0 after clear", vector_size(v) == 0);

    /* Test reserve */
    TEST("reserve returns 0", vector_reserve(v, 200) == 0);
    TEST("capacity >= 200 after reserve", vector_capacity(v) >= 200);

    /* Test shrink */
    for (int i = 0; i < 10; i++) vector_push(v, &i);
    TEST("shrink_to_fit returns 0", vector_shrink_to_fit(v) == 0);
    TEST("capacity == size after shrink", vector_capacity(v) == vector_size(v));

    /* Cleanup */
    vector_free(v, NULL);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}