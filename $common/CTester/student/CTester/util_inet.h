/**
 * @file util_inet.h
 * Utility functions related to htons, ntohs, htonl, ntohl
 */
/*
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

/**
 * Converts each short integer of the src array from host byte order
 * to network byte order, to the dest array.
 * @param src   source array of short integer in host byte order
 * @param dest  destination array of will-be short ints in network byte order
 * @param len   length of each array
 */
void htons_tab(uint16_t *src, uint16_t *dest, size_t len);

/**
 * Converts each short integer of the src array from network byte order
 * to host byte order, to the dest array.
 * @param src   source array of short integer in network byte order
 * @param dest  destination array of will-be short integers in host byte order
 * @param len   length of each array
 */
void ntohs_tab(uint16_t *src, uint16_t *dest, size_t len);

/**
 * Converts each integer of the src array from host byte order
 * to network byte order, to the dest array.
 * @param src   source array of integer in host byte order
 * @param dest  destination array of will-be integers in network byte order
 * @param len   length of each array
 */
void htonl_tab(uint32_t *src, uint32_t *dest, size_t len);

/**
 * Converts each integer of the src array from network byte order
 * to host byte order, to the dest array.
 * @param src   source array of integer in network byte order
 * @param dest  destination array of will-be integers in host byte order
 * @param len   length of each array
 */
void ntohl_tab(uint32_t *src, uint32_t *dest, size_t len);

#endif // __CTESTER_UTIL_INET_H__

