/*
 * Wrapper for htons, ntohs, htonl, ntohl, and other future functions.
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

#ifndef __WRAP_NETWORK_INET_H_
#define __WRAP_NETWORK_INET_H_

/**
 * Structures for htons, ntohs, htonl, ntohl.
 */

struct params_htons_t {
    uint16_t hostshort;
};
struct params_ntohs_t {
    uint16_t netshort;
};
struct params_htonl_t {
    uint32_t hostlong;
};
struct params_ntohl_t {
    uint32_t netlong;
};

struct stats_htons_t {
    int called;
    struct params_htons_t last_params;
    uint16_t last_return;
};
struct stats_ntohs_t {
    int called;
    struct params_ntohs_t last_params;
    uint16_t last_return;
};
struct stats_htonl_t {
    int called;
    struct params_htonl_t last_params;
    uint32_t last_return;
};
struct stats_ntohl_t {
    int called;
    struct params_ntohl_t last_params;
    uint32_t last_return;
};


/**
 * Utility functions and structures
 */
void reinit_network_inet_stats();


#endif // __WRAP_NETWORK_INET_H_

