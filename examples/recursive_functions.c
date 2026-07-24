/* recursive_functions.c — CiOpt Complexity Example
 * Expected results:
 *   factorial()          → O(n)     — Linear recursion
 *   fibonacci_naive()    → O(2ⁿ)    — Exponential! Missing memoization
 *   binary_search_rec()  → O(log n) — Divide and conquer
 */

/* O(n) — Linear recursion */
int factorial(int n)
{
    if (n <= 1)
        return 1;
    return n * factorial(n - 1);
}

/* O(2ⁿ) — Exponential! Missing memoization */
int fibonacci_naive(int n)
{
    if (n <= 1)
        return n;
    return fibonacci_naive(n - 1) + fibonacci_naive(n - 2);
}

/* O(log n) — Binary search (divide and conquer) */
int binary_search_rec(int *arr, int low, int high, int target)
{
    if (low > high)
        return -1;

    int mid = low + (high - low) / 2;

    if (arr[mid] == target)
        return mid;
    else if (arr[mid] < target)
        return binary_search_rec(arr, mid + 1, high, target);
    else
        return binary_search_rec(arr, low, mid - 1, target);
}