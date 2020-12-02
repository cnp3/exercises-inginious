/**
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

#ifndef __CTESTER_UTIL_INET_H__
#define __CTESTER_UTIL_INET_H__

#include <stddef.h>

void htons_tab(uint16_t *src, uint16_t *dest, size_t len);

void ntohs_tab(uint16_t *src, uint16_t *dest, size_t len);

void htonl_tab(uint32_t *src, uint32_t *dest, size_t len);

void ntohl_tab(uint32_t *src, uint32_t *dest, size_t len);

#endif // __CTESTER_UTIL_INET_H__

