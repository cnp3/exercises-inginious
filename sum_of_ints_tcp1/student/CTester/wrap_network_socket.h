/*
 * Wrapper for accept, bind, connect, listen, poll, select,
 * recv, recvfrom, recvmsg, send, sendto, sendmsg, socket.
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

#ifndef __WRAP_NETWORK_SOCKET_H_
#define __WRAP_NETWORK_SOCKET_H_

/**
 * Structures for accept.
 * In addition to the common stats structures,
 * there is an additional structure called return_accept_t,
 * which will retain the returned contained, in the case that the student
 * frees the allocated memory before we have the possibility to check it.
 */
struct params_accept_t {
    int sockfd;
    struct sockaddr *addr;
    socklen_t *addrlen_ptr;
};

/**
 * Structure to remember the returned value.
 * ret: return value of the call
 * addr: sockaddr filled in *addr in the call. Set to 0 if failure.
 * addrlen: size of the sockaddr filled in *addr, or zero if failure.
 */
struct return_accept_t {
    struct sockaddr_storage addr; // We use a sockaddr_storage as it is guaranteed to always be big enough to contain sockaddresses.
    socklen_t addrlen;
};

struct stats_accept_t {
    int called;
    struct params_accept_t last_params;
    int last_return;
    struct return_accept_t last_returns;
};

/**
 * Structures for bind
 */
struct params_bind_t {
    int sockfd;
    const struct sockaddr *addr;
    socklen_t addrlen;
};

struct stats_bind_t {
    int called;
    struct params_bind_t last_params;
    int last_return;
};


/**
 * Structures for connect
 */
struct params_connect_t {
    int sockfd;
    const struct sockaddr *addr;
    socklen_t addrlen;
};

struct stats_connect_t {
    int called;
    struct params_connect_t last_params;
    int last_return;
};


/**
 * Structures for listen
 */
struct params_listen_t {
    int sockfd;
    int backlog;
};

struct stats_listen_t {
    int called;
    struct params_listen_t last_params;
    int last_return;
};


/**
 * Structures for poll
 */
struct params_poll_t {
    struct pollfd *fds_ptr;
    unsigned long int nfds; // Strictly speaking, this is a nfds_t, but is is at least an unsigned long int.
    int timeout;
    struct pollfd *fds_copy;
};

struct stats_poll_t {
    int called;
    struct params_poll_t last_params;
    int last_return;
};


/**
 * Structures for recv, recvfrom, recvmsg.
 */
struct params_recv_t {
    int sockfd;
    void *buf;
    size_t len;
    int flags;
};
struct params_recvfrom_t {
    int sockfd;
    void *buf;
    size_t len;
    int flags;
    struct sockaddr *src_addr;
    socklen_t *addrlen_ptr;
};

struct params_recvmsg_t {
    int sockfd;
    struct msghdr *msg;
    int flags;
};
struct return_recvfrom_t {
    struct sockaddr_storage src_addr;
    socklen_t addrlen;
};

// TODO see if we can merge last_returns.ret and last_return as only one variable within the structure, and with only this simplified access.
struct stats_recv_t {
    int called;
    struct params_recv_t last_params;
    ssize_t last_return;
};
struct stats_recvfrom_t {
    int called;
    struct params_recvfrom_t last_params;
    ssize_t last_return;
    struct return_recvfrom_t last_returned_addr;
};
struct stats_recvmsg_t {
    int called;
    struct params_recvmsg_t last_params;
    ssize_t last_return;
    struct msghdr last_returned_msg;
};
struct stats_recv_all_t {
    int called; // Should be incremented each time we increment another one
};


/**
 * Structures for select.
 */

struct params_select_t {
    int nfds;
    fd_set *readfds_ptr;
    fd_set *writefds_ptr;
    fd_set *exceptfds_ptr;
    struct timeval *timeout_ptr;
    fd_set readfds;
    fd_set writefds;
    fd_set exceptfds;
    struct timeval timeout;
};

struct stats_select_t {
    int called;
    struct params_select_t last_params;
    int last_return;
};


/**
 * Structures for send, sendto and sendmsg.
 */
struct params_send_t {
    int sockfd;
    const void *buf;
    size_t len;
    int flags;
};
struct params_sendto_t {
    int sockfd;
    const void *buf;
    size_t len;
    int flags;
    const struct sockaddr *dest_addr_ptr;
    //struct sockaddr_storage dest_addr;
    socklen_t addrlen;
};
struct params_sendmsg_t {
    int sockfd;
    const struct msghdr *msg_ptr;
    //struct msghdr msg;
    int flags;
};

struct stats_send_t {
    int called;
    struct params_send_t last_params;
    ssize_t last_return;
};
struct stats_sendto_t {
    int called;
    struct params_sendto_t last_params;
    ssize_t last_return;
};
struct stats_sendmsg_t {
    int called;
    struct params_sendmsg_t last_params;
    ssize_t last_return;
};
struct stats_send_all_t {
    int called; // Should be incremented each time we increment another one.
};


/**
 * Structures for shutdown.
 */

struct params_shutdown_t {
    int sockfd;
    int how;
};

struct stats_shutdown_t {
    int called;
    struct params_shutdown_t last_params;
    int last_return;
};


/**
 * Structures for socket
 */
struct params_socket_t {
    int domain;
    int type;
    int protocol;
};

struct stats_socket_t {
    int called; // Number of times the function has been called
    struct params_socket_t last_params; // Arguments of the last call to socket(2)
    int last_return; // Return value of the last call to socket(2)
};


/**
 * Utility functions and structures
 */
void reinit_network_socket_stats();


#endif // __WRAP_NETWORK_SOCKET_H__
