/**
 * CiOpt Ultimate Stress Test
 * 
 * This file tests every detection capability of the analyzer.
 * Run: .\ciopt.exe analyze .\examples\ultimate_stress_test.c
 * 
 * Expected Detections:
 * - O(1), O(log n), O(n), O(n log n), O(n^2), O(n^3), O(2^n)
 * - Linear, Logarithmic, and Exponential Recursion
 * - Anti-patterns: strlen in loop, strcat in loop, unsafe gets()
 * - Dead code detection
 * - Known O(N) function calls inside loops
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * 1. O(1) - Constant Time
 * ============================================================ */
int add_numbers(int a, int b) {
    return a + b;
}

/* ============================================================
 * 2. O(log n) - Logarithmic Time (Halving pattern)
 * ============================================================ */
int binary_search_iterative(int *arr, int n, int target) {
    int low = 0, high = n - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2; // Halving pattern
        if (arr[mid] == target) return mid;
        if (arr[mid] < target) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

/* ============================================================
 * 3. O(n) - Linear Time
 * ============================================================ */
int sum_array(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/* ============================================================
 * 4. O(n log n) - Linearithmic Time 
 * (Loop containing a known O(log n) function call)
 * ============================================================ */
void process_with_bsearch(int *sorted_arr, int n, int *queries, int q_count) {
    for (int i = 0; i < q_count; i++) {
        // bsearch is O(log n). Loop is O(n). Total = O(n log n)
        bsearch(&queries[i], sorted_arr, n, sizeof(int), NULL); 
    }
}

/* ============================================================
 * 5. O(n^2) - Quadratic Time (Nested Loops)
 * ============================================================ */
void bubble_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/* ============================================================
 * 6. O(n^3) - Cubic Time (Triple Nested Loops)
 * ============================================================ */
void matrix_multiply_3d(int A[10][10], int B[10][10], int C[10][10], int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

/* ============================================================
 * 7. O(2^n) - Exponential Time (Tree Recursion)
 * ============================================================ */
int fibonacci_naive(int n) {
    if (n <= 1) return n;
    // Two recursive branches with overlapping subproblems
    return fibonacci_naive(n - 1) + fibonacci_naive(n - 2); 
}

/* ============================================================
 * 8. Recursion Patterns
 * ============================================================ */
// Linear Recursion: O(n)
int factorial(int n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}

// Logarithmic Recursion: O(log n)
long power_recursive(long base, int exp) {
    if (exp == 0) return 1;
    if (exp % 2 == 0) {
        long half = power_recursive(base, exp / 2); // Halving depth
        return half * half;
    }
    return base * power_recursive(base, exp - 1);
}

/* ============================================================
 * 9. Anti-Pattern: strlen() inside loop condition/body
 * ============================================================ */
void bad_string_iter(char *str) {
    // strlen is O(n). Called in loop condition makes it O(n^2)
    for (int i = 0; i < strlen(str); i++) { 
        if (str[i] == 'a') str[i] = 'b';
    }
}

/* ============================================================
 * 10. Anti-Pattern: strcat() inside loop
 * ============================================================ */
void bad_string_build(char *result, char **words, int count) {
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        strcat(result, words[i]); // O(n) inside O(n) loop = O(n^2)
        strcat(result, " ");
    }
}

/* ============================================================
 * 11. Anti-Pattern: Unsafe gets()
 * ============================================================ */
void unsafe_input(void) {
    char buffer[64];
    printf("Enter text: ");
    gets(buffer); // CRITICAL: Buffer overflow risk
    printf("You said: %s\n", buffer);
}

/* ============================================================
 * 12. Dead Code Detection
 * ============================================================ */
int unreachable_code(int x) {
    if (x > 0) {
        return x * 2;
        printf("This line is unreachable!\n"); // DEAD CODE
    }
    return 0;
}

/* ============================================================
 * 13. Known O(N) function called inside a loop
 * ============================================================ */
void loop_with_memcpy(int *dest, int *src, int n, int chunk_size) {
    for (int i = 0; i < n; i += chunk_size) {
        // memcpy is O(chunk_size). If chunk_size is related to n, it's O(n^2)
        memcpy(dest + i, src + i, chunk_size * sizeof(int)); 
    }
}

/* ============================================================
 * Main (Just to make it compile cleanly)
 * ============================================================ */
int main(void) {
    printf("CiOpt Ultimate Stress Test compiled successfully.\n");
    return 0;
}