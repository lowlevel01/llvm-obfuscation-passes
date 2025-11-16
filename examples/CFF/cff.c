#include <stdio.h>

int compute(int x) {
    int result = 0;

    if (x < 0) {
        result = -x;             
    } else if (x == 0) {
        result = 42;            
    } else {
        result = x;            
    }

    for (int i = 0; i < 3; i++) {
        result += i;          
    }

    switch (x % 3) {         
        case 0: result += 10; break;
        case 1: result += 20; break;
        default: result += 30; break;
    }

    return result;               
}

int main() {
    for (int i = -1; i <= 2; i++) {
        printf("compute(%d) = %d\n", i, compute(i));
    }
    return 0;
}

