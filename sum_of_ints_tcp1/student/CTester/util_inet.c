#include <arpa/inet.h>

#include "util_inet.h"

void htons_tab(uint16_t *src, uint16_t *dest, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        dest[i] = htons(src[i]);
    }
}

void ntohs_tab(uint16_t *src, uint16_t *dest, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        dest[i] = ntohs(src[i]);
    }
}

void htonl_tab(uint32_t *src, uint32_t *dest, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        dest[i] = htonl(src[i]);
    }
}

void ntohl_tab(uint32_t *src, uint32_t *dest, size_t len)
{
    for (unsigned i = 0; i < len; i++) {
        dest[i] = ntohl(src[i]);
    }
}

