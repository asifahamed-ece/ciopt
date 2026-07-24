/* simple_loops.c — CiOpt Complexity Example
 * Expected results:
 *   constant_time()      → O(1)
 *   linear_search()      → O(n)
 *   sum_array()          → O(n)
 *   constant_loop()      → O(1)  (fixed 10 iterations)
 *   binary_search_like() → O(log n)
 */

/* O(1) — Constant time */
int constant_time(int a, int b)
{
    return a + b;
}

/* O(n) — Single loop over input */
int linear_search(int *arr, int n, int target)
{
    for (int i = 0; i < n; i++) {
        if (arr[i] == target)
            return i;
    }
    return -1;
}

/* O(n) — Summation */
int sum_array(int *arr, int n)
{
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/* O(1) — Constant bound loop */
int constant_loop(void)
{
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    return sum;
}

/* O(log n) — Halving pattern */
int binary_search_like(int n)
{
    int count = 0;
    while (n > 1) {
        n /= 2;
        count++;
    }
    return count;
}