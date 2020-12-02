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

#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>

#include "read_write.h"
#include "wrap.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


int     __real_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     __real_bind   (int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     __real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     __real_listen(int sockfd, int backlog);
int     __real_poll(struct pollfd *fds, nfds_t nfds, int timeout);
ssize_t __real_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t __real_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t __real_recvmsg(int sockfd, struct msghdr *msg, int flags);
int     __real_select(int nfds, fd_set *readfds, fd_set *write_fds, fd_set *except_fds, struct timeval *timeout);
ssize_t __real_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t __real_sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t __real_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int     __real_shutdown(int sockfd, int how);
int     __real_socket(int domain, int type, int protocol);


extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;

/**
 * Auxiliary functions and structures for use by implementers for read/write
 */
extern bool fd_is_read_buffered(int fd);

extern ssize_t read_handle_buffer(int fd, void *buf, size_t len, int flags);

extern struct read_fd_table_t read_fd_table;


/**
 * Wrap functions.
 */


/*
 * Note: addr should point to an existing sockaddr, or NULL if we don't want it
 * and addrlen should point to an existing socklen_t containing the size
 * of the existing sockaddr, and will be modified to reflect the true size
 * of the returned sockaddr; it may be greater than the original value
 * if the provided sockaddr is too small, and this sockaddr will be truncated.
 * If addr in NULL, addrlen should also be NULL.
 */
int __wrap_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    if (!(wrap_monitoring && monitored.accept)) {
        return __real_accept(sockfd, addr, addrlen);
    }
    socklen_t old_addrlen = (addrlen == NULL ? 0 : *addrlen); // May crash if addrlen doesn't point to a valid address.
    stats.accept.called++;
    stats.accept.last_params = (struct params_accept_t) {
        .sockfd = sockfd,
        .addr = addr,
        .addrlen_ptr = addrlen
    };
    // Reinit stats
    stats.accept.last_returns.addrlen = 0;
    memset(&(stats.accept.last_returns.addr), 0, sizeof(struct sockaddr_storage));
    if (FAIL(failures.accept)) {
        failures.accept = NEXT(failures.accept);
        errno = failures.accept_errno;
        return (stats.accept.last_return = failures.accept_ret);
    }
    failures.accept = NEXT(failures.accept);
    int ret = -2;
    ret = __real_accept(sockfd, addr, addrlen);
    if (ret == 0 && addr != NULL) {
        /*
         * We should only copy the returned address
         * - if there was no error (otherwise it is probably not wise)
         * - if addr != NULL; if addr == NULL, then whatever addrlen is,
         *   nothing was returned
         */
        stats.accept.last_returns.addrlen = (addrlen == NULL ? 0 : *addrlen);
        /*
         * We need the minimum as *addrlen may be greater than old_addrlen
         * and grater than the size of *addr, and we're not interested
         * in potential garbage not set by accept in the remaining space.
         * This can't segfault as ret == 0
         */
        memcpy(&(stats.accept.last_returns.addr), addr, MIN(old_addrlen, *addrlen));
    } // else: there was an error, so the safest thing to do is ignore the value provided
    return (stats.accept.last_return = ret);
}

int __wrap_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (!(wrap_monitoring && monitored.bind)) {
        return __real_bind(sockfd, addr, addrlen);
    }
    stats.bind.called++;
    stats.bind.last_params = (struct params_bind_t) {
        .sockfd = sockfd,
        .addr = addr,
        .addrlen = addrlen
    };
    if (FAIL(failures.bind)) {
        failures.bind = NEXT(failures.bind);
        errno = failures.bind_errno;
        return (stats.bind.last_return = failures.bind_ret);
    }
    failures.bind = NEXT(failures.bind);
    int ret = -2;
    ret = __real_bind(sockfd, addr, addrlen);
    return (stats.bind.last_return = ret);
}

int __wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (!(wrap_monitoring && monitored.connect)) {
        return __real_connect(sockfd, addr, addrlen);
    }
    stats.connect.called++;
    stats.connect.last_params = (struct params_connect_t) {
        .sockfd = sockfd,
        .addr = addr,
        .addrlen = addrlen
    };
    if (FAIL(failures.connect)) {
        failures.connect = NEXT(failures.connect);
        errno = failures.connect_errno;
        return (stats.connect.last_return = failures.connect_ret);
    }
    failures.connect = NEXT(failures.connect);
    int ret = -2;
    ret = __real_connect(sockfd, addr, addrlen);
    return (stats.connect.last_return = ret);
}

int __wrap_listen(int sockfd, int backlog)
{
    if (!(wrap_monitoring && monitored.listen)) {
        return __real_listen(sockfd, backlog);
    }
    stats.listen.called++;
    stats.listen.last_params = (struct params_listen_t) {
        .sockfd = sockfd,
        .backlog = backlog
    };
    if (FAIL(failures.listen)) {
        failures.listen = NEXT(failures.listen);
        errno = failures.listen_errno;
        return (stats.listen.last_return = failures.listen_ret);
    }
    failures.listen = NEXT(failures.listen);
    int ret = -2;
    ret = __real_listen(sockfd, backlog);
    return (stats.listen.last_return = ret);
}

int __wrap_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    if (!(wrap_monitoring && monitored.poll)) {
        return __real_poll(fds, nfds, timeout);
    }
    stats.poll.called++;
    stats.poll.last_params = (struct params_poll_t) {
        .fds_ptr = fds,
        .nfds = nfds,
        .timeout = timeout,
        .fds_copy = NULL
    };
    if (FAIL(failures.poll)) {
        failures.poll = NEXT(failures.poll);
        errno = failures.poll_errno;
        return (stats.poll.last_return = failures.poll_ret);
    }
    failures.poll = NEXT(failures.poll);
    int ret = -2;
    ret = __real_poll(fds, nfds, timeout);
    if (! (ret == -1 && errno == EFAULT)) {
        struct pollfd *tmp = malloc(nfds * sizeof(struct pollfd));
        if (tmp) {
            memcpy(fds, tmp, nfds * sizeof(struct pollfd));
            stats.poll.last_params.fds_copy = tmp;
        }
    }
    return ret;
}

ssize_t __wrap_recv(int sockfd, void *buf, size_t len, int flags)
{
    if (!(wrap_monitoring && monitored.recv)) {
        return __real_recv(sockfd, buf, len, flags);
    }
    stats.recv.called++;
    stats.recv_all.called++;
    stats.recv.last_params = (struct params_recv_t) {
        .sockfd = sockfd,
        .buf = buf,
        .len = len,
        .flags = flags
    };
    if (FAIL(failures.recv)) {
        failures.recv = NEXT(failures.recv);
        errno = failures.recv_errno;
        return (stats.recv.last_return = failures.recv_ret);
    }
    failures.recv = NEXT(failures.recv);
    ssize_t ret = -1;
    if (fd_is_read_buffered(sockfd)) {
        ret = read_handle_buffer(sockfd, buf, len, flags);
    } else {
        ret = __real_recv(sockfd, buf, len, flags);
    }
    return (stats.recv.last_return = ret);
}

ssize_t __wrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    if (!(wrap_monitoring && monitored.recvfrom)) {
        return __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    }
    socklen_t old_addrlen = (addrlen == NULL ? 0 : *addrlen); // May crash
    stats.recvfrom.called++;
    stats.recv_all.called++;
    stats.recvfrom.last_params = (struct params_recvfrom_t) {
        .sockfd = sockfd,
        .buf = buf,
        .len = len,
        .flags = flags,
        .src_addr = src_addr,
        .addrlen_ptr = addrlen
    };
    stats.recvfrom.last_returned_addr.addrlen = 0;
    memset(&(stats.recvfrom.last_returned_addr.src_addr), 0, sizeof(struct sockaddr_storage));
    if (FAIL(failures.recvfrom)) {
        failures.recvfrom = NEXT(failures.recvfrom);
        errno = failures.recvfrom_errno;
        return (stats.recv.last_return = failures.recvfrom_ret);
    }
    failures.recvfrom = NEXT(failures.recvfrom);
    ssize_t ret = -1;
    if (fd_is_read_buffered(sockfd) && src_addr == NULL && addrlen == NULL) {
        ret = read_handle_buffer(sockfd, buf, len, flags);
    } else {
        ret = __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    }
    if (ret >= 0 && src_addr != NULL) {
        // Same justification as in accept
        stats.recvfrom.last_returned_addr.addrlen = *addrlen;
        memcpy(&(stats.recvfrom.last_returned_addr.src_addr), src_addr, MIN(old_addrlen, *addrlen));
    }
    return (stats.recvfrom.last_return = ret);
}

ssize_t __wrap_recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    if (!(wrap_monitoring && monitored.recvmsg)) {
        return __real_recvmsg(sockfd, msg, flags);
    }
    stats.recvmsg.called++;
    stats.recv_all.called++;
    stats.recvmsg.last_params = (struct params_recvmsg_t) {
        .sockfd = sockfd,
        .msg = msg,
        .flags = flags
    };
    // Reinit struct
    memset(&(stats.recvmsg.last_returned_msg), 0, sizeof(struct msghdr));
    if (FAIL(failures.recvmsg)) {
        failures.recvmsg = NEXT(failures.recvmsg);
        errno = failures.recvmsg_errno;
        return (stats.recvmsg.last_return = failures.recvmsg_ret);
    }
    failures.recvmsg = NEXT(failures.recvmsg);
    ssize_t ret = -1;
    ret = __real_recvmsg(sockfd, msg, flags);
    if (ret == 0) {
        // Assume that msg doesn't point to an invalid location
        memcpy(&(stats.recvmsg.last_returned_msg), msg, sizeof(struct msghdr));
    }
    return ret;
}

int __wrap_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    if (!(wrap_monitoring && monitored.select)) {
        return __real_select(nfds, readfds, writefds, exceptfds, timeout);
    }
    stats.select.called++;
    stats.select.last_params = (struct params_select_t) {
        .nfds = nfds,
        .readfds_ptr = readfds,
        .writefds_ptr = writefds,
        .exceptfds_ptr = exceptfds,
        .timeout_ptr = timeout,
        .readfds = *readfds, // FIXME may cause a segfault if the student passed garbage
        .writefds = *writefds,
        .exceptfds = *exceptfds,
        .timeout = *timeout
    };
    if (FAIL(failures.select)) {
        failures.select = NEXT(failures.select);
        errno = failures.select_errno;
        return (stats.select.last_return = failures.select_ret);
    }
    failures.select = NEXT(failures.select);
    int ret = -1;
    ret = __real_select(nfds, readfds, writefds, exceptfds, timeout);
    return ret;
}

ssize_t __wrap_send(int sockfd, const void *buf, size_t len, int flags)
{
    if (!(wrap_monitoring && monitored.send)) {
        return __real_send(sockfd, buf, len, flags);
    }
    stats.send.called++;
    stats.send_all.called++;
    stats.send.last_params = (struct params_send_t) {
        .sockfd = sockfd,
        .buf = buf,
        .len = len,
        .flags = flags
    };
    if (FAIL(failures.send)) {
        failures.send = NEXT(failures.send);
        errno = failures.send_errno;
        return (stats.send.last_return = failures.send_ret);
    }
    failures.send = NEXT(failures.send);
    ssize_t ret = -1;
    ret = __real_send(sockfd, buf, len, flags);
    return (stats.send.last_return = ret);
}

ssize_t __wrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    if (!(wrap_monitoring && monitored.sendto)) {
        return __real_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    }
    stats.sendto.called++;
    stats.send_all.called++;
    stats.sendto.last_params = (struct params_sendto_t) {
        .sockfd = sockfd,
        .buf = buf,
        .len = len,
        .flags = flags,
        .dest_addr_ptr = dest_addr,
        //.dest_addr = (struct sockaddr_storage)0,
        .addrlen = addrlen
    };
    /*if (dest_addr != NULL) {
        memcpy(&(stats.sendto.last_params.dest_addr), dest_addr, MIN(addrlen, sizeof(struct sockaddr_storage))); // May segfault if dest_addr is badly defined.
    }*/
    if (FAIL(failures.sendto)) {
        failures.sendto = NEXT(failures.sendto);
        errno = failures.sendto_errno;
        return (stats.sendto.last_return = failures.sendto_ret);
    }
    failures.sendto = NEXT(failures.sendto);
    ssize_t ret = -1;
    ret = __real_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    return (stats.sendto.last_return = ret);
}

ssize_t __wrap_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    if (!(wrap_monitoring && monitored.sendmsg)) {
        return __real_sendmsg(sockfd, msg, flags);
    }
    stats.sendmsg.called++;
    stats.send_all.called++;
    stats.sendmsg.last_params = (struct params_sendmsg_t) {
        .sockfd = sockfd,
        .msg_ptr = msg,
        //.msg = (struct msghdr)0,
        .flags = flags
    };
    /*if (msg != NULL) {
        memcpy(&(stats.sendmsg.last_params.msg), msg, sizeof(struct msghdr));
    }*/
    if (FAIL(failures.sendmsg)) {
        failures.sendmsg = NEXT(failures.sendmsg);
        errno = failures.sendmsg_errno;
        return (stats.sendmsg.last_return = failures.sendmsg_ret);
    }
    failures.sendmsg = NEXT(failures.sendmsg);
    ssize_t ret = -1;
    ret = __real_sendmsg(sockfd, msg, flags);
    return ret;
}

int __wrap_shutdown(int sockfd, int how)
{
    if (!(wrap_monitoring && monitored.shutdown)) {
        return __real_shutdown(sockfd, how);
    }
    stats.shutdown.called++;
    stats.shutdown.last_params = (struct params_shutdown_t) {
        .sockfd = sockfd,
        .how = how
    };
    if (FAIL(failures.shutdown)) {
        failures.shutdown = NEXT(failures.shutdown);
        errno = failures.shutdown_errno;
        return (stats.shutdown.last_return = failures.shutdown_ret);
    }
    failures.shutdown = NEXT(failures.shutdown);
    int ret = -1;
    ret = __real_shutdown(sockfd, how);
    return ret;
}

int __wrap_socket(int domain, int type, int protocol)
{
    if (!(wrap_monitoring && monitored.socket)) {
        return __real_socket(domain, type, protocol);
    }
    stats.socket.called++;
    stats.socket.last_params = (struct params_socket_t) {
        .domain = domain,
        .type = type,
        .protocol = protocol
    };
    if (FAIL(failures.socket)) {
        failures.socket = NEXT(failures.socket);
        errno = failures.socket_errno;
        return (stats.socket.last_return = failures.socket_ret);
    }
    failures.socket = NEXT(failures.socket);
    int ret = -2;
    ret = __real_socket(domain, type, protocol);
    return (stats.socket.last_return = ret);
}

// Additionnal functions

void reinit_network_socket_stats()
{
    memset(&(stats.accept), 0, sizeof(struct stats_accept_t));
    memset(&(stats.bind), 0, sizeof(struct stats_bind_t));
    memset(&(stats.connect), 0, sizeof(struct stats_connect_t));
    memset(&(stats.listen), 0, sizeof(struct stats_listen_t));
    memset(&(stats.poll), 0, sizeof(stats.poll));
    memset(&(stats.recv), 0, sizeof(struct stats_recv_t));
    memset(&(stats.recvfrom), 0, sizeof(struct stats_recvfrom_t));
    memset(&(stats.recvmsg), 0, sizeof(struct stats_recvmsg_t));
    memset(&(stats.recv_all), 0, sizeof(struct stats_recv_all_t));
    memset(&(stats.select), 0, sizeof(stats.select));
    memset(&(stats.send), 0, sizeof(struct stats_send_t));
    memset(&(stats.sendto), 0, sizeof(struct stats_sendto_t));
    memset(&(stats.sendmsg), 0, sizeof(struct stats_sendmsg_t));
    memset(&(stats.send_all), 0, sizeof(struct stats_send_all_t));
    memset(&(stats.shutdown), 0, sizeof(stats.shutdown));
    memset(&(stats.socket), 0, sizeof(struct stats_socket_t));
}

