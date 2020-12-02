/**
 * @file wrap_network_dns.h
 * Wrappers for getaddrinfo, freeaddrinfo, gai_strerror and getnameinfo,
 * and other miscellaneous helper functions.
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

#ifndef __WRAP_NETWORK_DNS_H_
#define __WRAP_NETWORK_DNS_H_

#include <netdb.h>
#include <stdbool.h>


// getaddrinfo parameters structures and statistics structure

/**
 * Parameters structure for getaddrinfo
*/

struct params_getaddrinfo_t {
    const char *node;
    const char *service;
    const struct addrinfo *hints;
    struct addrinfo **res;
};

// Used to record the addrinfo lists "returned" by getaddrinfo, in order to check for their deallocation via freeaddrinfo.
/**
 * Node for a list of all returned lists of struct addrinfo
 * returned by getaddrinfo.
 * This is used to check that they are deallocated via freeaddrinfo correctly.
 */
struct addrinfo_node_t {
    struct addrinfo *addr_list;
    struct addrinfo_node_t *next;
};

/**
 * Statistics structure for getaddrinfo
 */
struct stats_getaddrinfo_t {
    int called; // number of times the getaddrinfo call has been issued
    struct params_getaddrinfo_t last_params; // parameters for the last monitored call
    struct addrinfo_node_t *addrinfo_list; // list of addrinfo lists "returned" by getaddrinfo
    int last_return; // return value of the last monitored call issued
};


// getnameinfo parameters structures and statistics structure

/**
 * Parameters structure for getnameinfo
 */
struct params_getnameinfo_t {
    const struct sockaddr *addr;
    socklen_t addrlen;
    char *host;
    socklen_t hostlen;
    char *serv;
    socklen_t servlen;
    int flags;
};

/**
 * Statistics structure for getnameinfo
 */
struct stats_getnameinfo_t {
    int called; // number of calls
    struct params_getnameinfo_t last_params; // parameters of the last monitored call
    int last_return; // return value of the last monitored call issued
};


// freeaddrinfo parameters structures and statistics structure

/**
 * @struct params_freeaddrinfo_t
 * Parameter structure for freeaddrinfo.
 * There is no such structure, as there is only one parameter.
 */

/**
 * Statistics structure for freeaddrinfo
 */
struct stats_freeaddrinfo_t {
    int called; /**< number of calls */
    struct addrinfo *last_param; /**< parameter of the last monitored call issued */
    /**
     * Status of the last monitored call:
     * if check_freeaddrinfo is true
     *     and last_param was not returned by getaddrinfo during the monitoring,
     *     then status is 1;
     * else, status is zero.
     */
    int status;
};


// gai_strerror parameters structures and statistics structure

/**
 * @struct params_gai_strerror_t
 * Parameter structure for gai_strerror.
 * There is no such structure, as there is only one parameter.
 */

/**
 * Statistics structure for gai_strerror
 */
struct stats_gai_strerror_t {
    int called; ///< number of calls
    int last_params; ///< parameter of the last monitored call
    const char *last_return; ///< return value of the last monitored call
};

// TODO Maybe add some init, clean and resetstats methods to these calls ?

/**
 * Resets the statistics of the DNS-related methods.
 */
void reinit_stats_network_dns();


/**
 * Enforces checking that the passed structure has been previously returned by getaddrinfo.
 * When check is true, freeaddrinfo will check if its argument has been "returned"
 * by an earlier call to getaddrinfo directly, and will report if it is not the case.
 * @see freeaddrinfo_badarg_report_t
 *
 * @param check do we enforce checking?
 */
void set_check_freeaddrinfo(bool check);


/**
 * Functions of this type should return in *res a valid list of
 * struct addrinfo, in a manner compatible with getaddrinfo,
 * suitable to be freed by a function of type freeaddrinfo_method_t.
 */
typedef int (*getaddrinfo_method_t)(const char *node, const char *service,
        const struct addrinfo *hints, struct addrinfo **res);

/**
 * Functions of this type should be able to free structures allocated
 * by a function of type getaddrinfo_method_t.
 */
typedef void (*freeaddrinfo_method_t)(struct addrinfo *res);

/**
 * Specifies the actual function called by the getaddrinfo wrapper.
 *
 * If method is not NULL, replaces __real_getaddrinfo by the function "method"
 * as the real function executed by the wrapper.
 * If method is NULL, resets the method to __real_getaddrinfo.
 * This method only sets the substitute for the getaddrinfo function,
 * and not the substitute for the freeaddrinfo function;
 * incompatible definitions of these functions may result in memleaks
 * or double free if care is not taken.
 * @see set_gai_methods()
 *
 * @param method the function called by the wrapper.
 */
void set_getaddrinfo_method(getaddrinfo_method_t method);

/**
 * Sets the getaddrinfo and freeaddrinfo replacement functions
 * to be used in the sandbox.
 *
 * Specifying NULL for a parameter means that the system will use
 * the default, system-defined method (__real_getaddrinfo or
 * __real_freeaddrinfo).
 * This is especially needed as the POSIX standard doesn't specify the way
 * the list of struct addrinfo is generated: glibc allocates a single block
 * of memory for both the struct addrinfo and its struct sockaddr,
 * as well as one block of memory for the canonical name (if asked for).
 * Therefore, calling __real_freeaddrinfo on a custom-built list may result
 * in memleaks, and using a wrong freeaddrinfo_method_t to free a list
 * generated by __real_getaddrinfo may cause a double free.
 * @see set_getaddrinfo_method()
 *
 * @param gai_method the function called by the getaddrinfo wrapper.
 * @param fai_method the function called by the freeaddrinfo wrapper.
 */
void set_gai_methods(getaddrinfo_method_t gai_method, freeaddrinfo_method_t fai_method);


/**
 * Functions of this type should return in host and serv valid names for a host
 * and service given the address addr, in manner compatible with getnameinfo.
 */
typedef int (*getnameinfo_method_t)(const struct sockaddr *addr,
        socklen_t addrlen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);

/**
 * Sets the getnameinfo replacement function to be used in the sandbox.
 *
 * If method is not NULL, replaces __real_getnameinfo by the function "method"
 * as the real function executed by the wrapper.
 * If method is NULL, resets the method to __real_getnameinfo.
 * @see set_getaddrinfo_method().
 *
 * @param method the function called by the getnameinfo wrapper.
 */
void set_getnameinfo_method(getnameinfo_method_t method);


/**
 * Functions of this type should return a string given an error code
 * for the getaddrinfo function call.
 * The string should not be freed by the caller.
 */
typedef const char *(*gai_strerror_method_t)(int);

/**
 * Sets the gai_strerror replacement function to be used in the sandbox.
 *
 * If method is not NULL, replaces __real_gai_strerror by the function "method"
 * as the real function called by the wrapper.
 * If method is NULL, resets the method to __real_gai_strerror.
 * This allows specifying other error messages than the standard ones.
 *
 * @param method the function called by the gai_strerror wrapper.
 */
void set_gai_strerror_method(gai_strerror_method_t method);


/**
 * Functions of this type will be called when freeaddrinfo is called with
 * an argument that has never been returned by the getaddrinfo wrapper,
 * when freeaddrinfo is configured to check the validity of its argument.
 * @see set_check_freeaddrinfo()
 */
typedef void (*freeaddrinfo_badarg_report_t)();

/**
 * Sets the method to be called if check_freeaddrinfo is true
 * and the argument passed to freeaddrinfo was not returned by getaddrinfo.
 *
 * This is useful if you want to use the push_info_msg method
 * for your feedback, for example. Default is NULL.
 *
 * @param reporter the function to be called when a bad argument is provided
 *                 to freeaddrinfo and detected.
 */
void set_freeaddrinfo_badarg_report(freeaddrinfo_badarg_report_t reporter);


/**
 * Resets all methods to the default, library-provided ones.
 */
void reset_gai_fai_gstr_gni_methods();

/**
 * Replaces getaddrinfo by a version which doesn't call DNS.
 * node should be a numerical address (like 192.168.1.1),
 * which will be converted using inet_pton and stored inside the answer.
 * serv should be a positive short integer as a string, representing the port.
 * Returns only one addrinfo in *res, with appropriate values taken from
 * node, serv and the fields from hints. Also, the canonical name is set,
 * if specified, by 'C' followed by the content of node.
 * If node is NULL and hints->ai_flags contains AI_PASSIVE, then IPv4 (0.0.0.0)
 * is used if hints->ai_family is AF_INET, otherwise IPv6 (::) is used.
 * If hints is NULL, an address with SOCK_DGRAM as its socket type is returned.
 * @see simple_freeaddrinfo for the corresponding freeaddrinfo_method_t
 * @warning This function is probably only minimally tested, and would benefit
 *          from more testng.
 * @param node  See the standard getaddrinfo
 * @param serv  See the standard getaddrinfo
 * @param hints See the standard getaddrinfo
 * @param res   See the standard getaddrinfo
 */
int simple_getaddrinfo(const char *node, const char *serv, const struct addrinfo *hints, struct addrinfo **res);

/**
 * Necessary for cross-library compatibility: should be used in place
 * of the default freeaddrinfo to free the list returned by simple_getaddrinfo;
 * see set_gai_methods for more information.
 * @see simple_getaddrinfo for the corresponding creator function.
 * @param res See the standard freeaddrinfo
 */
void simple_freeaddrinfo(struct addrinfo *res);

#endif // __WRAP_NETWORK_DNS_H_

