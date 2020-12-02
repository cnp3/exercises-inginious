/*
 * Utility functions related to htons, ntohs, htonl, ntohl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

