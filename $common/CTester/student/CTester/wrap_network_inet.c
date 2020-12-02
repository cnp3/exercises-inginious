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

#include <string.h>

#include "wrap.h"

uint16_t __real_htons(uint16_t hostshort);
uint16_t __real_ntohs(uint16_t netshort);
uint32_t __real_htonl(uint32_t hostlong);
uint32_t __real_ntohl(uint32_t netlong);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;

uint16_t __wrap_htons(uint16_t hostshort)
{
    uint16_t ret = __real_htons(hostshort);
    if (wrap_monitoring && monitored.htons) {
        stats.htons.called++;
        stats.htons.last_params.hostshort = hostshort;
        stats.htons.last_return = ret;
    }
    return ret;
}

uint16_t __wrap_ntohs(uint16_t netshort)
{
    uint16_t ret = __real_ntohs(netshort);
    if (wrap_monitoring && monitored.ntohs) {
        stats.ntohs.called++;
        stats.ntohs.last_params.netshort = netshort;
        stats.ntohs.last_return = ret;
    }
    return ret;
}

uint32_t __wrap_htonl(uint32_t hostlong)
{
    uint32_t ret = __real_htonl(hostlong);
    if (wrap_monitoring && monitored.htonl) {
        stats.htonl.called++;
        stats.htonl.last_params.hostlong = hostlong;
        stats.htonl.last_return = ret;
    }
    return ret;
}

uint32_t __wrap_ntohl(uint32_t netlong)
{
    uint32_t ret = __real_ntohl(netlong);
    if (wrap_monitoring && monitored.ntohl) {
        stats.ntohl.called++;
        stats.ntohl.last_params.netlong = netlong;
        stats.ntohl.last_return = ret;
    }
    return ret;
}

// Additionnal functions

void reinit_network_inet_stats()
{
    memset(&(stats.htons), 0, sizeof(stats.htons));
    memset(&(stats.ntohs), 0, sizeof(stats.ntohs));
    memset(&(stats.htonl), 0, sizeof(stats.htonl));
    memset(&(stats.ntohl), 0, sizeof(stats.ntohl));
}

