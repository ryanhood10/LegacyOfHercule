// Freestanding helpers the compiler may call for struct copies/clears.
#include "rom.h"

void *memcpy(void *dst, const void *src, unsigned n) {
    u8 *d = dst;
    const u8 *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memset(void *dst, int c, unsigned n) {
    u8 *d = dst;
    while (n--) *d++ = (u8)c;
    return dst;
}
