// Write a code that demonstrates code time complexity in C17, similar to the Fibonacci example provided. This code will implement a function that calculates the factorial of a number using recursion, which has a time complexity of O(n).
#include <stdio.h>

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int n = 10;
    printf("Calculating factorial(%d)...\n", n);
    int result = factorial(n);
    printf("Result: %d\n", result);
    return 0;
}
