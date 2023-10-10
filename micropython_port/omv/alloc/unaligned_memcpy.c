#include <stdint.h>
#include <string.h>
#include "unaligned_memcpy.h"

void *unaligned_memcpy(void *dest, void *src, size_t n) {
    void *ret = dest;

    if (((uint64_t)src & 7) == 0 && ((uint64_t)dest & 7) == 0) {
        uint64_t *dest64 = (uint64_t *)dest;
        uint64_t *src64 = (uint64_t *)src;
        for (; n > 8; n -= 8)
            *dest64++ = *src64++;
        dest = dest64;
        src = src64;
    }
    if (n)
        memcpy(dest, src, n);
    return ret;
}
