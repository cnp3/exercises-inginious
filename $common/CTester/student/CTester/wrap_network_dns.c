/*
 * Wrapper for getaddrinfo, freeaddrinfo, gai_strerror and getnameinfo
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include "wrap.h"

int __real_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
int __real_getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
void __real_freeaddrinfo(struct addrinfo *res);
const char *__real_gai_strerror(int errcode);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;

bool check_freeaddrinfo = false;
getaddrinfo_method_t  getaddrinfo_method  = NULL;
freeaddrinfo_method_t freeaddrinfo_method = NULL;
getnameinfo_method_t  getnameinfo_method  = NULL;
gai_strerror_method_t gai_strerror_method = NULL;
freeaddrinfo_badarg_report_t freeaddrinfo_badarg_reporter = NULL;


void add_result(struct addrinfo *res)
{
    struct addrinfo_node_t *new_node = malloc(sizeof(*new_node));
    if (new_node == NULL) return;
    new_node->addr_list = res;
    new_node->next = stats.getaddrinfo.addrinfo_list;
    stats.getaddrinfo.addrinfo_list = new_node;
}

/**
 * Returns 0 if res was successfully removed of the list,
 * -1 if res was not present in the list,
 *  -2 if there was some other errors.
 */
int remove_result(struct addrinfo *res)
{
    if (stats.getaddrinfo.addrinfo_list != NULL) {
        struct addrinfo_node_t *runner = stats.getaddrinfo.addrinfo_list;
        if (runner->addr_list == res) {
            stats.getaddrinfo.addrinfo_list = runner->next;
            free(runner);
            // runner->addr_list will be freed by freeaddrinfo
            return 0;
        }
        for (; runner->next != NULL; runner = runner->next) {
            if (runner->next->addr_list == res) {
                struct addrinfo_node_t *old = runner->next;
                runner->next = runner->next->next;
                free(old);
                // old->addr_list will be freed by freeaddrinfo when we return
                return 0;
            }
        }
        return -1;
    } else {
        return -1;
    }
}


int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    if (! (wrap_monitoring && monitored.getaddrinfo)) {
        return __real_getaddrinfo(node, service, hints, res);
    }
    stats.getaddrinfo.called++;
    stats.getaddrinfo.last_params = (struct params_getaddrinfo_t) {
        .node = node,
        .service = service,
        .hints = hints,
        .res = res
    };
    if (FAIL(failures.getaddrinfo)) {
        failures.getaddrinfo = NEXT(failures.getaddrinfo);
        errno = failures.getaddrinfo_errno;
        stats.getaddrinfo.last_return = failures.getaddrinfo_ret;
        return failures.getaddrinfo_ret;
    }
    failures.getaddrinfo = NEXT(failures.getaddrinfo);
    int ret;
    if (getaddrinfo_method == NULL) {
        ret = __real_getaddrinfo(node, service, hints, res);
    } else {
        ret = (*getaddrinfo_method)(node, service, hints, res);
    }
    stats.getaddrinfo.last_return = ret;
    /*
     * At this point, we may assume that *res is a valid memory location,
     * as the program will segfault if it is not.
     */
    if (check_freeaddrinfo) {
        add_result(*res);
    }
    return ret;
}

int __wrap_getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
        char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
    if (! (wrap_monitoring && monitored.getnameinfo)) {
        return __real_getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
    }
    stats.getnameinfo.called++;
    stats.getnameinfo.last_params = (struct params_getnameinfo_t) {
        .addr = addr,
        .addrlen = addrlen,
        .host = host,
        .hostlen = hostlen,
        .serv = serv,
        .servlen = servlen,
        .flags = flags
    };
    if (FAIL(failures.getnameinfo)) {
        failures.getnameinfo = NEXT(failures.getnameinfo);
        errno = failures.getnameinfo_errno;
        stats.getnameinfo.last_return = failures.getnameinfo_ret;
        return failures.getnameinfo_ret;
    }
    failures.getnameinfo = NEXT(failures.getnameinfo);
    getnameinfo_method_t met = &__real_getnameinfo;
    if (getnameinfo_method != NULL) {
        met = getnameinfo_method;
    }
    int ret = (*met)(addr, addrlen, host, hostlen, serv, servlen, flags);
    stats.getnameinfo.last_return = ret;
    return ret;
}

/**
 * The student should call freeaddrinfo on all the result structures that have been initialized by getaddrinfo; failing to do so leads to memory leak.
 * If the parameter check_freeaddrinfo is true, then this function will check if res was actually returned by getaddrinfo, and is not either an invalid pointer, or a non-first addrinfo in one of the lists.
 */
void __wrap_freeaddrinfo(struct addrinfo *res)
{
    if (! (wrap_monitoring && monitored.freeaddrinfo)) {
        __real_freeaddrinfo(res);
        return;
    }
    stats.freeaddrinfo.called++;
    stats.freeaddrinfo.last_param = res;
    if (check_freeaddrinfo) {
        if (remove_result(res) == 0) {
            // Okay, it is valid
            stats.freeaddrinfo.status = 0;
        } else {
            // Not okay: it is invalid (e.g. this is the second item of the list instead of the first).
            // TODO report the error, as it is still invalid
            stats.freeaddrinfo.status = 1;
            if (freeaddrinfo_badarg_reporter != NULL) {
                (*freeaddrinfo_badarg_reporter)();
            }
        }
    } else {
        stats.freeaddrinfo.status = 0; // will not check
    }
    if (freeaddrinfo_method == NULL) {
        __real_freeaddrinfo(res);
    } else {
        freeaddrinfo_method(res);
    }
}

const char * __wrap_gai_strerror(int ecode)
{
    if (! (wrap_monitoring && monitored.gai_strerror)) {
        return __real_gai_strerror(ecode);
    }
    stats.gai_strerror.called++;
    stats.gai_strerror.last_params = ecode;
    gai_strerror_method_t met = (gai_strerror_method == NULL ? &__real_gai_strerror : gai_strerror_method);
    const char *ret = (*met)(ecode);
    stats.gai_strerror.last_return = ret;
    return ret;
}


void reinit_stats_network_dns()
{
    // First, clean up the list of getaddrinfo returned lists
    struct addrinfo_node_t *runner = stats.getaddrinfo.addrinfo_list;
    while (runner != NULL) {
        struct addrinfo_node_t *old = runner;
        runner = runner->next;
        /* Ideally, we should also free old->addr_list by using freeaddrinfo,
           but as it is possible that the list should be deallocated
           by a different function than the current freeaddrinfo,
           it is safer to not attempt it and leave it as-is.
           TODO: add the dedicated freeaddrinfo as a field of the nodes ;-)
        */
        free(old);
    }
    memset(&(stats.getaddrinfo), 0, sizeof(stats.getaddrinfo));
    memset(&(stats.freeaddrinfo), 0, sizeof(stats.freeaddrinfo));
    memset(&(stats.getnameinfo), 0, sizeof(stats.getnameinfo));
    memset(&(stats.gai_strerror), 0, sizeof(stats.gai_strerror));
}

void set_check_freeaddrinfo(bool check)
{
    check_freeaddrinfo = check;
}


void set_getaddrinfo_method(getaddrinfo_method_t method)
{
    getaddrinfo_method = method;
}

void set_gai_methods(getaddrinfo_method_t gai_method, freeaddrinfo_method_t fai_method)
{
    getaddrinfo_method  = gai_method;
    freeaddrinfo_method = fai_method;
}

void set_getnameinfo_method(getnameinfo_method_t method)
{
    getnameinfo_method = method;
}

void set_gai_strerror_method(gai_strerror_method_t method)
{
    gai_strerror_method = method;
}


void set_freeaddrinfo_badarg_report(freeaddrinfo_badarg_report_t reporter)
{
    freeaddrinfo_badarg_reporter = reporter;
}

void reset_gai_fai_gstr_gni_methods()
{
    set_gai_methods(NULL, NULL);
    set_getnameinfo_method(NULL);
    set_gai_strerror_method(NULL);
}

int simple_getaddrinfo(const char *node, const char *serv, const struct addrinfo *hints, struct addrinfo **res)
{
    if (node == NULL && hints != NULL && hints && (hints->ai_flags & AI_CANONNAME) != 0) {
        return EAI_BADFLAGS;
    }
    if (node == NULL && serv == NULL) {
        return EAI_NONAME;
    }
    uint8_t buf[sizeof(struct in6_addr)];
    memset(buf, 0, sizeof(buf));
    int family = hints ? hints->ai_family : AF_UNSPEC;
    if (node == NULL) {
        if (hints && (hints->ai_flags & AI_PASSIVE)) {
            if (family == AF_INET) {
                node = "0.0.0.0";
            } else {
                node = "::";
            }
        } else {
            if (family == AF_INET) {
                node = "127.0.0.1";
            } else {
                node = "::1";
            }
        }
    }
    // Let's try IPv4
    int s = inet_pton(AF_INET, node, buf);
    if (s == -1) { // error; should not happen
        return EAI_FAMILY;
    } else if (s == 1) { // IPVv4
        family = AF_INET;
    } else { // Let's try IPv6
        memset(buf, 0, sizeof(buf));
        s = inet_pton(AF_INET6, node, buf);
        if (s == 1) {
            family = AF_INET6;
        } else if (s == -1) { // error; should not happen
            return EAI_FAMILY;
        } else { // badly encoded then
            return EAI_NONAME;
        }
    }
    char *endservptr = NULL;
    in_port_t port = (serv == NULL) ? 0 : htons((uint16_t)(strtol(serv, &endservptr, 10)));
    if (serv != NULL && (endservptr == NULL || *endservptr != '\0')) {
        // As we assume that we only deal with numeric hosts and service numbers, this is invalid
        return EAI_NONAME;
    }
    if (hints && hints->ai_family != AF_UNSPEC && family != hints->ai_family)
        return EAI_FAMILY;
    // Converted, and family identified
    *res = malloc(sizeof(struct addrinfo));
    struct addrinfo *rp = *res;
    if (rp == NULL)
        return EAI_MEMORY;
    rp->ai_flags = 0;
    rp->ai_family = family;
    rp->ai_socktype = hints ? hints->ai_socktype : SOCK_DGRAM;
    rp->ai_protocol = hints ? hints->ai_protocol : 0;
    if (hints && (hints->ai_flags & AI_CANONNAME) != 0) {
        rp->ai_canonname = malloc(strlen(node) + 2);
        if (rp->ai_canonname == NULL) {
            free(rp);
            *res = NULL;
            return EAI_MEMORY;
        }
        strcpy((rp->ai_canonname) + 1, node);
        rp->ai_canonname[0] = 'C';
    } else {
        rp->ai_canonname = NULL;
    }
    rp->ai_next = NULL;
    if (family == AF_INET) {
        struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
        if (addr == NULL) {
            free(rp->ai_canonname);
            free(rp);
            *res = NULL;
            return EAI_MEMORY;
        }
        addr->sin_family = AF_INET;
        addr->sin_port = port;
        memcpy(&(addr->sin_addr), buf, sizeof(struct in_addr));
        rp->ai_addr = (struct sockaddr*)addr;
        rp->ai_addrlen = sizeof(struct sockaddr_in);
    } else {
        struct sockaddr_in6 *addr = malloc(sizeof(struct sockaddr_in6));
        if (addr == NULL) {
            free(rp->ai_canonname);
            free(rp);
            *res = NULL;
            return EAI_MEMORY;
        }
        memset(addr, 0, sizeof(*addr));
        addr->sin6_family = AF_INET6;
        addr->sin6_port = port;
        memcpy(&(addr->sin6_addr), buf, sizeof(struct in6_addr));
        rp->ai_addr = (struct sockaddr*)addr;
        rp->ai_addrlen = sizeof(struct sockaddr_in6);
    }
    return 0;
}

void simple_freeaddrinfo(struct addrinfo *res)
{
    struct addrinfo *p = res;
    while (p != NULL) {
        struct addrinfo *old = p;
        p = p->ai_next;
        free(old->ai_canonname);
        free(old->ai_addr);
        free(old);
    }
}
