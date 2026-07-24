/* nested_loops.c — CiOpt Complexity Example
 * Expected results:
 *   bubble_sort()    → O(n²)
 *   matrix_multiply() → O(n³)
 *   print_pairs()    → O(n²)
 */

/* O(n²) — Bubble sort (nested loops) */
void bubble_sort(int *arr, int n)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

/* O(n³) — Matrix multiply (triple nested loops) */
void matrix_multiply(int n, int a[n][n], int b[n][n], int c[n][n])
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            c[i][j] = 0;
            for (int k = 0; k < n; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

/* O(n²) — Print all pairs */
void print_pairs(int *arr, int n)
{
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("(%d, %d)\n", arr[i], arr[j]);
        }
    }
}