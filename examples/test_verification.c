/*
 * test_verification.c - Final verification of CiOpt complexity analysis
 * 
 * Expected results:
 *   constant_work()         -> O(1)
 *   logarithmic_shift()     -> O(log n)
 *   linear_scan()           -> O(n)
 *   nlogn_nested_halving()  -> O(n log n)
 *   nlogn_with_bsearch()    -> O(n log n)
 *   quadratic_pair()        -> O(n^2)
 *   cubic_triple()          -> O(n^3)
 *   fibonacci_bad()         -> O(2^n)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* O(1) - Constant time */
int constant_work(int a, int b) {
    int c = a + b;
    int d = c * 2;
    return d - a;
}

/* O(log n) - Right-shift halving */
int logarithmic_shift(int n) {
    int count = 0;
    for (int j = n; j > 0; j >>= 1) {
        count++;
    }
    return count;
}

/* O(n) - Simple linear scan */
int linear_scan(int *arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/* O(n log n) - Linear outer loop, halving inner loop */
void nlogn_nested_halving(int n) {
    int count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = n; j > 0; j = j / 2) {
            count++;
        }
    }
}

/* O(n log n) - Linear loop calling O(log n) bsearch */
void nlogn_with_bsearch(int *arr, int n, int *queries, int q) {
    for (int i = 0; i < q; i++) {
        bsearch(&queries[i], arr, n, sizeof(int), NULL);
    }
}

/* O(n^2) - Classic nested linear loops */
void quadratic_pair(int *arr, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (arr[i] > arr[j]) {
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

/* O(n^3) - Triple nested linear loops */
void cubic_triple(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            for (int k = 0; k < n; k++) {
                sum += i * j * k;
            }
        }
    }
}

/* O(2^n) - Naive fibonacci recursion */
int fibonacci_bad(int n) {
    if (n <= 1) return n;
    return fibonacci_bad(n - 1) + fibonacci_bad(n - 2);
}

int main() {
    return 0;
}
