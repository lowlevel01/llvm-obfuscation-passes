#include <stdint.h>
#include <string.h>
#include <stdlib.h>


char* decrypt_string_xor(char* str, int len, uint8_t key) {
    for (int i = 0; i < len; i++) {
        str[i] ^= key;
    }
    return str;
}
