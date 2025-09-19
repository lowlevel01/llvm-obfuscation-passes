#include <stdio.h>

const char* global_str = "Global string\n";  

void test_func() {
    printf("Function string\n");            
    const char* local = "Local string\n";  
    printf("%s\n", local);
}

int main() {
    printf("Main string\n");              
    test_func();
    return 0;
}
