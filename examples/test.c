/* test_nlogn.c - Test for O(n log n) detection */
#include <stdio.h>

void executeNLogN(int n) {
    int operationCount = 0;

    // Outer loop runs 'n' times (Linear time complexity: O(n))
    for (int i = 0; i < n; i++) {

        // Inner loop halves 'j' on each step (Logarithmic time complexity: O(log n))
        for (int j = n; j > 0; j = j / 2) {
            operationCount++;
        }
    }

    printf("For n = %d, total operations executed = %d\n", n, operationCount);
}

int main() {
    int n = 8;
    executeNLogN(n);

    n = 16;
    executeNLogN(n);

    return 0;
}