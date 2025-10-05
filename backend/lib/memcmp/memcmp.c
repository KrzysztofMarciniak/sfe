#include "memcmp.h"

extern int memcmp(const void* a, const void* b, size_t len) {
        const unsigned char* p1 = (const unsigned char*)a;
        const unsigned char* p2 = (const unsigned char*)b;
        unsigned char diff      = 0;
        for (size_t i = 0; i < len; ++i) {
                diff |= p1[i] ^ p2[i];
        }
        return diff;
}
