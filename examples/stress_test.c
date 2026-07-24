/**
 * CiOpt Stress Test File
 * ----------------------
 * This file is designed to trigger every analysis feature in CiOpt.
 * Run: .\ciopt.exe analyze .\examples\stress_test.c -v
 *
 * Expected detections:
 *   - O(1)    : swap, is_even
 *   - O(n)    : reverse_array, count_chars
 *   - O(log n): binary_search, power_fast
 *   - O(n^2)  : selection_sort, build_string_bad
 *   - O(n^3)  : floyd_warshall
 *   - O(2^n)  : subset_sum
 *   - Anti-patterns: strcat in loop, gets usage, strlen in condition
 *   - Dead code after return
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * O(1) - Constant Time
 * ============================================================ */

void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int is_even(int n)
{
    return n % 2 == 0;
}

/* ============================================================
 * O(n) - Linear Time
 * ============================================================ */

void reverse_array(int *arr, int n)
{
    for (int i = 0; i < n / 2; i++) {
        int tmp = arr[i];
        arr[i] = arr[n - 1 - i];
        arr[n - 1 - i] = tmp;
    }
}

int count_chars(const char *str, char target)
{
    int count = 0;
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == target)
            count++;
        i++;
    }
    return count;
}

/* ============================================================
 * O(log n) - Logarithmic Time
 * ============================================================ */

int binary_search(int *arr, int n, int target)
{
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        if (arr[mid] == target) return mid;
        else if (arr[mid] < target) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

long power_fast(long base, int exp)
{
    long result = 1;
    while (exp > 0) {
        if (exp % 2 == 1)
            result *= base;
        base *= base;
        exp /= 2;
    }
    return result;
}

/* ============================================================
 * O(n^2) - Quadratic Time
 * ============================================================ */

void selection_sort(int *arr, int n)
{
    for (int i = 0; i < n - 1; i++) {
        int min_idx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[min_idx])
                min_idx = j;
        }
        swap(&arr[i], &arr[min_idx]);
    }
}

/* Anti-pattern: strcat inside loop -> O(n^2) string building */
void build_string_bad(char *result, const char **words, int count)
{
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(result, words[i]);
        strcat(result, " ");
    }
}

/* ============================================================
 * O(n^3) - Cubic Time
 * ============================================================ */

#define MAX_NODES 100
#define INF 999999

void floyd_warshall(int dist[MAX_NODES][MAX_NODES], int n)
{
    for (int k = 0; k < n; k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (dist[i][k] + dist[k][j] < dist[i][j])
                    dist[i][j] = dist[i][k] + dist[k][j];
            }
        }
    }
}

/* ============================================================
 * O(2^n) - Exponential Time (Recursion)
 * ============================================================ */

int subset_sum(int *set, int n, int target)
{
    if (target == 0) return 1;
    if (n == 0) return 0;

    if (set[n - 1] > target)
        return subset_sum(set, n - 1, target);

    return subset_sum(set, n - 1, target) ||
           subset_sum(set, n - 1, target - set[n - 1]);
}

/* ============================================================
 * Anti-patterns & Dead Code
 * ============================================================ */

/* Anti-pattern: gets() is unsafe */
void read_input_bad(void)
{
    char buffer[256];
    printf("Enter text: ");
    gets(buffer);
    printf("You said: %s\n", buffer);
}

/* Dead code: unreachable statement after return */
int dead_code_example(int x)
{
    if (x > 0) {
        return x * 2;
        printf("This line is unreachable!\n");
    }
    return 0;
}

/* Anti-pattern: strlen in loop condition -> O(n^2) */
void to_uppercase_bad(char *str)
{
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] -= 32;
    }
}

/* ============================================================
 * Main - runs everything
 * ============================================================ */

int main(void)
{
    /* O(1) tests */
    int a = 5, b = 10;
    swap(&a, &b);
    printf("swap: a=%d, b=%d\n", a, b);
    printf("is_even(4) = %d\n", is_even(4));

    /* O(n) tests */
    int arr[] = {1, 2, 3, 4, 5};
    reverse_array(arr, 5);
    printf("reversed[0] = %d\n", arr[0]);
    printf("count 'l' in hello = %d\n", count_chars("hello", 'l'));

    /* O(log n) tests */
    int sorted[] = {1, 3, 5, 7, 9, 11, 13};
    printf("search 7 = index %d\n", binary_search(sorted, 7, 7));
    printf("2^10 = %ld\n", power_fast(2, 10));

    /* O(n^2) test */
    int unsorted[] = {64, 25, 12, 22, 11};
    selection_sort(unsorted, 5);
    printf("sorted[0] = %d\n", unsorted[0]);

    /* O(2^n) test */
    int set[] = {3, 34, 4, 12, 5, 2};
    printf("subset_sum(9) = %d\n", subset_sum(set, 6, 9));

    /* Dead code */
    printf("dead_code_example(5) = %d\n", dead_code_example(5));

    return 0;
}
