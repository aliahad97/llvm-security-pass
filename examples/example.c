#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int sum_to(int n) {
    int s = 0;
    for (int i = 0; i <= n; i++) {
        s += i;
    }
    return s;
}

int main(void) {
    printf("%d\n", add(2, 3));
    printf("%d\n", sum_to(10));
    return 0;
}
