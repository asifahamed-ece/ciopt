#include <stdio.h>

// Recursive function with O(2^n) time complexity
long long fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    // Each call branches into two more recursive calls
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = 45; // Even a small number like 45 will take a noticeably long time
    
    printf("Calculating Fibonacci(%d)...\n", n);
    long long result = fibonacci(n);
    printf("Result: %lld\n", result);
    
    return 0;
}
