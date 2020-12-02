#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>

#include "util_sockets.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

const uint8_t _OK = 0;
const uint8_t _TOO_MUCH = 1;
const uint8_t _TOO_FEW = 2;
const uint8_t _RECV_ERROR = 3;
const uint8_t _NOTHING_RECV = 4;
const uint8_t _SEND_ERROR = 8;
const uint8_t _NOT_SAME_DATA = 16;
const uint8_t _NOT_SAME_ADDR = 32;
const uint8_t _EXIT_PROCESS = 64;
const uint8_t _EXTEND_MSG = 128;

int create_socket(const char *host, const char *serv, int domain, int type, int flags, int do_bind)
{
    errno = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | flags;
    hints.ai_family = domain;
    hints.ai_socktype = type;
    struct addrinfo *rep;
    int r = getaddrinfo(host, serv, &hints, &rep);
    if (r || !rep) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
        return -1;
    }
    int fd = socket(rep->ai_family, rep->ai_socktype, rep->ai_protocol);
    if (fd == -1) {
        perror("socket");
        freeaddrinfo(rep);
        return -1;
    }
    if (do_bind) {
        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
            perror("setsockopt");
        }
        if (bind(fd, rep->ai_addr, rep->ai_addrlen)) {
            perror("bind");
            close(fd);
            freeaddrinfo(rep);
            return -1;
        }
    } else {
        if (connect(fd, rep->ai_addr, rep->ai_addrlen)) {
            perror("connect");
            close(fd);
            freeaddrinfo(rep);
            return -1;
        }
    }
    freeaddrinfo(rep);
    return fd;
}

int create_tcp_server_socket(const char *serv, int domain, int type)
{
    int serverfd = create_socket(NULL, serv, domain, type, AI_PASSIVE, true);
    if (listen(serverfd, 1)) {
        perror("listen");
        close(serverfd);
        return -1;
    }
    return serverfd;
}

int create_tcp_client_socket(const char *host, const char *serv, int domain, int type)
{
    int clientfd = create_socket(host, serv, domain, type, 0, false);
    return clientfd;
}

int create_udp_server_socket(const char *serv, int domain)
{
    int serverfd = create_socket(NULL, serv, domain, SOCK_DGRAM, AI_PASSIVE, true);
    return serverfd;
}

int create_udp_client_socket(const char *host, const char *serv, int domain)
{
    int clientfd = create_socket(host, serv, domain, SOCK_DGRAM, 0, false);
    return clientfd;
}

int connect_udp_server_to_client(int sfd, int cfd)
{
    // send from cfd to recvfrom sfd.
    char dumb = 'A';
    ssize_t s = send(cfd, &dumb, sizeof(dumb), 0);
    if (s != sizeof(dumb)) {
        perror("send");
        return -1;
    }
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    ssize_t r = recvfrom(sfd, &dumb, sizeof(dumb), 0, (struct sockaddr*)&client_addr, &client_addrlen);
    if (r != sizeof(dumb)) {
        perror("recvfrom");
        return -1;
    }
    if (connect(sfd, (struct sockaddr*)&client_addr, client_addrlen)) {
        perror("connect");
        return -1;
    }
    return 0;
}

#define PIPE_AND_FORK_BEGIN() \
if (pipe(c2s)) { \
    perror("pipe"); \
    return -1; \
} \
if (pipe(s2c)) { \
    perror("pipe"); \
    close(c2s[0]); \
    close(c2s[1]); \
    return -1; \
} \
int pid = fork(); \
if (pid == -1) { \
    perror("fork"); \
    close(c2s[0]); \
    close(c2s[1]); \
    close(s2c[0]); \
    close(s2c[1]); \
    return -1; \
} else if (pid == 0) { \
    close(c2s[1]); \
    close(s2c[0]); \
    /* server : end of macro, start of custom */

#define PIPE_AND_FORK_MIDDLE() \
    /* server : end of custom, start of macro */ \
} else { \
    close(c2s[0]); \
    close(s2c[1]); \
    /* client : end of macro, start of custom */

#define PIPE_AND_FORK_END() \
    /* client : end of custom */ \
}

/**
 * Returns 0 if correct,
 * -3 if syscall problem
 * -2 if pipe_in got a problem,
 * -1 if recv error
 *  1 if pipe_in got a message
 *  2 if too few data recv'd
 *  3 if too much data recv'd
 *  4 if different data
 * Writes of errors left to calling function, except for 2, 3.
 */
int wait_recv_tcp(int sockfd, int pipe_in, int pipe_out, const struct cs_network_chunk *chunk)
{
    size_t required_bytes = chunk->data_length, recv_bytes = 0, total_bytes = required_bytes + 1;
    ssize_t r = -1;
    void *buf = malloc(total_bytes);
    if (!buf) {
        perror("malloc");
        return -3;
    }
    nfds_t nfds = 2;
    struct pollfd fds[2];
    fds[0] = (struct pollfd) {
        .fd = sockfd,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    fds[1] = (struct pollfd) {
        .fd = pipe_in,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    do {
        int p = poll(fds, nfds, -1);
        if (p == -1) {
            perror("poll");
            free(buf);
            return -3;
        } // p != -1
        if (fds[1].revents) {
            // Got something on pipe
            if (fds[1].revents & POLLIN) {
                // Read; quit
                free(buf);
                return 1;
            } else {
                // Error: quit
                free(buf);
                return -2;
            }
        } // not fds[1], so fds[0]
        if (fds[0].revents) {
            // Got something on socket
            if (fds[0].revents & POLLIN) {
                r = recv(sockfd, (buf + recv_bytes), (total_bytes - recv_bytes), 0);
                if (r == -1) {
                    // Error condition on socket
                    perror("recv");
                    free(buf);
                    return -1;
                }
                recv_bytes += r;
            } else {
                // Error condition on socket
                free(buf);
                return -1;
            }
        }
        // r != -1 and no error
    } while (recv_bytes <= required_bytes && r != 0);
    if (recv_bytes < required_bytes) {
        free(buf);
        write(pipe_out, &_TOO_FEW, U8SZ);
        write(pipe_out, &recv_bytes, sizeof(recv_bytes));
        return 2;
    }
    if (recv_bytes > required_bytes) {
        free(buf);
        write(pipe_out, &_TOO_MUCH, U8SZ);
        write(pipe_out, &recv_bytes, sizeof(recv_bytes));
        return 3;
    }
    int c = memcmp(buf, chunk->data, chunk->data_length);
    free(buf);
    if (c) {
        return 4;
    }
    return 0;
}

/**
 * Returns
 * -3 if syscall problem
 * -2 if pipe_in got a problem
 * -1 if send got a problem
 *  0 if ok
 *  1 if pipe_in got a message
 * Writes of error left to calling function, except for -1.
 */
int wait_send_tcp(int sockfd, int pipe_in, int pipe_out, const struct cs_network_chunk *chunk)
{
    size_t required_bytes = chunk->data_length, sent_bytes = 0;
    ssize_t s = -1;
    nfds_t nfds = 2;
    struct pollfd fds[2];
    fds[0] = (struct pollfd) {
        .fd = sockfd,
        .events = POLLOUT | POLLERR,
        .revents = 0
    };
    fds[1] = (struct pollfd) {
        .fd = pipe_in,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    do {
        int p = poll(fds, nfds, -1);
        if (p == -1) {
            perror("poll");
            return -3;
        }
        if (fds[1].revents) {
            if (fds[1].revents & POLLIN) {
                return 1;
            } else {
                return -2;
            }
        }
        if (fds[0].revents) {
            if (fds[0].revents & POLLOUT) {
                s = send(sockfd, (chunk->data + sent_bytes), (required_bytes - sent_bytes), 0);
                if (s == -1) {
                    perror("send");
                    write(pipe_out, &_SEND_ERROR, U8SZ);
                    return -1;
                }
                sent_bytes += s;
            } else {
                write(pipe_out, &_SEND_ERROR, U8SZ);
                return -1;
            }
        }
    } while (sent_bytes != required_bytes && s != -1);
    if (sent_bytes != required_bytes) {
        uint8_t err = _TOO_FEW | SEND_ERROR;
        write(pipe_out, &err, U8SZ);
        return -1;
    }
    return 0;
}

/**
 * Returns 0 if correct,
 * -3 if syscall problem
 * -2 if pipe_in got a problem,
 * -1 if recv error
 *  1 if pipe_in got a message
 *  2 if too few data recv'd
 *  3 if too much data recv'd
 *  4 if different data
 *  5 if different address
 * Writes of errors left to calling function, except for 2, 3, 4, 5.
 */
int wait_recv_udp(int sockfd, int pipe_in, int pipe_out, const struct cs_network_chunk *chunk, const struct sockaddr *addr, socklen_t addrlen)
{
    size_t required_bytes = chunk->data_length, total_bytes = required_bytes + 1, recv_bytes;
    struct sockaddr_storage recv_addr;
    socklen_t recv_addrlen = sizeof(addr);
    void *buf = malloc(total_bytes);
    if (!buf) {
        perror("malloc");
        return -3;
    }
    nfds_t nfds = 2;
    struct pollfd fds[2];
    fds[0] = (struct pollfd) {
        .fd = sockfd,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    fds[1] = (struct pollfd) {
        .fd = pipe_in,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    int p = poll(fds, nfds, -1);
    if (p == -1) {
        perror("poll");
        free(buf);
        return -3;
    }
    if (fds[1].revents) {
        // pipe_in
        if (fds[1].revents & POLLIN) {
            // Read: quit with 1
            free(buf);
            return 1;
        } else {
            // Error: quit with error
            free(buf);
            return -2;
        }
    }
    if (fds[0].revents) {
        // sockfd
        if (fds[0].revents & POLLIN) {
            // read socket
            ssize_t r = recvfrom(sockfd, buf, total_bytes, 0, (struct sockaddr*)&recv_addr, &recv_addrlen);
            if (r == -1) {
                perror("recvfrom");
                free(buf);
                return -1;
            }
            recv_bytes = r;
        } else {
            free(buf);
            return -1;
        }
    }
    if (recv_bytes < required_bytes) {
        free(buf);
        write(pipe_out, &_TOO_FEW, U8SZ);
        write(pipe_out, &recv_bytes, sizeof(recv_bytes));
        return 2;
    }
    if (recv_bytes > required_bytes) {
        free(buf);
        write(pipe_out, &_TOO_MUCH, U8SZ);
        write(pipe_out, &recv_bytes, sizeof(recv_bytes));
        return 3;
    }
    int c = memcmp(buf, chunk->data, chunk->data_length);
    free(buf);
    if (c) {
        write(pipe_out, &_NOT_SAME_DATA, U8SZ);
        return 4;
    }
    if (addr != NULL) {
        c = memcmp(&addr, &recv_addr, MIN(recv_addrlen, addrlen));
        if (c) {
            write(pipe_out, &_NOT_SAME_ADDR, U8SZ);
            write(pipe_out, &recv_addrlen, sizeof(recv_addrlen));
            write(pipe_out, &recv_addr, recv_addrlen);
            return 5;
        }
    }
    return 0;
}

/**
 * Returns
 * -3 if syscall problem
 * -2 if pipe_in got a problem
 * -1 if send got a problem
 *  0 if ok
 *  1 if pipe_in got a message
 * Writes of errors left to caller function, except for -1 (send).
 */
int wait_send_udp(int sockfd, int pipe_in, int pipe_out, const struct cs_network_chunk *chunk, const struct sockaddr *addr, socklen_t addrlen)
{
    size_t required_bytes = chunk->data_length, sent_bytes = 0;
    nfds_t nfds = 2;
    struct pollfd fds[2];
    fds[0] = (struct pollfd) {
        .fd = sockfd,
        .events = POLLOUT | POLLERR,
        .revents = 0
    };
    fds[1] = (struct pollfd) {
        .fd = pipe_in,
        .events = POLLIN | POLLERR,
        .revents = 0
    };
    int p = poll(fds, nfds, -1);
    if (p == -1) {
        perror("poll");
        return -3;
    }
    if (fds[1].revents) {
        // pipe_in
        if (fds[1].revents & POLLIN) {
            return 1;
        } else {
            return -2;
        }
    }
    if (fds[0].revents) {
        // sockfd
        if (fds[0].revents & POLLOUT) {
            ssize_t s = sendto(sockfd, chunk->data, required_bytes, 0, addr, addrlen);
            if (s == -1) {
                perror("sendto");
                write(pipe_out, &_SEND_ERROR, U8SZ);
                return -1;
            }
            sent_bytes = s;
        } else {
            write(pipe_out, &_SEND_ERROR, U8SZ);
            return -1;
        }
    }
    if (sent_bytes != required_bytes) {
        uint8_t err = _TOO_FEW | _SEND_ERROR;
        write(pipe_out, &err, U8SZ);
        return -1;
    }
    return 0;
}

/**
 * Return 3 if process was asked to stop, 2 if error, 0 on success.
 */
int handle_tcp_transaction(int sockfd, const struct cs_network_transaction *transaction, int pipe_in, int pipe_out)
{
    for (unsigned j = 0; j < transaction->nchunks; j++) {
        struct cs_network_chunk *curchunk = &(transaction->chunks[j]);
        if (curchunk->type == RECV_CHUNK) {
            // recv
            int r = wait_recv_tcp(sockfd, pipe_in, pipe_out, curchunk);
            if (r < -1) {
                fprintf(stderr, "Server error\n");
                return 2;
            }
            switch (r) {
                case -1:
                    // recv error
                    write(pipe_out, &_RECV_ERROR, U8SZ);
                    write(pipe_out, &errno, sizeof(errno));
                    continue;
                case 1:
                    // Stop the server
                    return 3;
                case 2:
                    // Not enough bytes before connection was closed
                    continue;
                case 3:
                    // Sent too much data
                    continue;
                case 4:
                    // Different data
                    write(pipe_out, &_NOT_SAME_DATA, U8SZ);
                    //write(pipe_out, &(res->expected_request_length), sizeof(res->expected_request_length));
                    // So that the reader can skip the data without knowing its length in advance
                    //write(pipe_out, buf, res->expected_request_length);
                    continue;
            }
        } else if (curchunk->type == SEND_CHUNK) {
            // send
            int s = wait_send_tcp(sockfd, pipe_in, pipe_out, curchunk);
            if (s < -1) {
                fprintf(stderr, "Server error\n");
                return 2;
            }
            if (s == -1) {
                // Send error
                continue;
            }
            if (s == 1) {
                return 3;
            }
        } // end if type
    } // end for chunks in transaction
    write(pipe_out, &_OK, U8SZ);
    return 0;
}

int handle_udp_transaction(int sockfd, const struct cs_network_transaction *transaction, int pipe_in, int pipe_out)
{
    for (unsigned int j = 0; j < transaction->nchunks; j++) {
        struct cs_network_chunk *curchunk = &(transaction->chunks[j]);
        if (curchunk->type == RECV_CHUNK) {
            // recvfrom
            int r = wait_recv_udp(sockfd, pipe_in, pipe_out, curchunk, transaction->addr, transaction->addrlen);
            if (r < -1) {
                fprintf(stderr, "Server error\n");
                return 2;
            }
            switch (r) {
                case -1:
                    //recv error
                    write(pipe_out, &_RECV_ERROR, U8SZ);
                    write(pipe_out, &errno, sizeof(errno));
                    continue;
                case 1:
                    // pipe_in in
                    return 3;
                // Else: handled by wait_recv_udp
                case 2: // too few
                    continue;
                case 3: // too much
                    continue;
                case 4: // data
                    continue;
                case 5: // addr
                    continue;
            }
            // TODO complete
        } else if (curchunk->type == SEND_CHUNK) {
            // sendto
            int r = wait_send_udp(sockfd, pipe_in, pipe_out, curchunk, transaction->addr, transaction->addrlen);
            if (r < -1) {
                fprintf(stderr, "Server error\n");
                return 2;
            }
            switch (r) {
                case -1: // send error
                    continue;
                case 1: // pipe_in in
                    return 3;
            }
        } // end if type
    } // end for chunks in transaction
    write(pipe_out, &_OK, U8SZ);
    return 0;
}

int launch_test_tcp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid)
{
    int c2s[2], s2c[2];
    PIPE_AND_FORK_BEGIN();
    // c2s[0] and s2c[1]
    int sfd = create_tcp_server_socket(serv, domain, SOCK_STREAM);
    if (sfd == -1) {
        write(s2c[1], &_EXIT_PROCESS, U8SZ);
        exit(1);
    }
    for (unsigned i = 0; i < transactions->ntransactions; i++) {
        struct cs_network_transaction *curtrans = &(transactions->transactions[i]);
        // First, accept the connection
        struct sockaddr_storage client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int cfd = accept(sfd, (struct sockaddr*)&client_addr, &client_addrlen);
        if (cfd == -1) {
            perror("accept");
            write(s2c[1], &_EXIT_PROCESS, U8SZ);
            exit(1);
        }
        int h = handle_tcp_transaction(cfd, curtrans, c2s[0], s2c[1]);
        switch (h) {
            case 0:
                break;
            case 2:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(2);
            case 3:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(3);
        }
    } // end for transaction in transactions
    exit(0);
    PIPE_AND_FORK_MIDDLE();
    // s2c[0] and c2s[1]
    *spid = pid;
    *server_in = c2s[1];
    *server_out = s2c[0];
    return 0;
    PIPE_AND_FORK_END();
}

int launch_test_tcp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid)
{
    int c2s[2], s2c[2];
    PIPE_AND_FORK_BEGIN();
    // c2s[0] and s2c[1]
    int cfd = create_tcp_client_socket(host, serv, domain, SOCK_STREAM);
    if (cfd == -1) {
        write(s2c[1], &_EXIT_PROCESS, U8SZ);
        exit(1);
    }
    uint8_t buf_in[1];
    for (unsigned i = 0; i < transactions->ntransactions; i++) {
        struct cs_network_transaction *curtrans = &(transactions->transactions[i]);
        // First, wait until the server is up
        ssize_t rr = read(c2s[0], buf_in, sizeof(buf_in));
        if (rr == -1) {
            perror("read");
            write(s2c[1], &_EXIT_PROCESS, U8SZ);
            exit(2);
        }
        int h = handle_tcp_transaction(cfd, curtrans, c2s[0], s2c[1]);
        switch (h) {
            case 0:
                break;
            case 2:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(2);
            case 3:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(3);
        }
    } // end for transaction in transactions
    exit(0);
    PIPE_AND_FORK_MIDDLE();
    // s2c[0] and c2s[1]
    *cpid = pid;
    *client_in = c2s[1];
    *client_out = s2c[0];
    return 0;
    PIPE_AND_FORK_END();
}

int launch_test_udp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid)
{
    int c2s[2], s2c[2];
    PIPE_AND_FORK_BEGIN();
    // c2s[0] and s2c[1]
    int sfd = create_udp_server_socket(serv, domain);
    if (sfd == -1) {
        write(s2c[1], &_EXIT_PROCESS, U8SZ);
        exit(1);
    }
    for (unsigned i = 0; i < transactions->ntransactions; i++) {
        struct cs_network_transaction *curtrans = &(transactions->transactions[i]);
        int h = handle_udp_transaction(sfd, curtrans, c2s[0], s2c[1]);
        switch (h) {
            case 0:
                break;
            case 2:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(2);
            case 3:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(3);
        }
    }
    exit(0);
    PIPE_AND_FORK_MIDDLE();
    *spid = pid;
    *server_in = c2s[1];
    *server_out = s2c[0];
    return 0;
    PIPE_AND_FORK_END();
}

int launch_test_udp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid)
{
    int c2s[2], s2c[2];
    PIPE_AND_FORK_BEGIN();
    int cfd = create_udp_client_socket(host, serv, domain);
    if (cfd == -1) {
        write(s2c[1], &_EXIT_PROCESS, U8SZ);
        exit(1);
    }
    for (unsigned i = 0; i < transactions->ntransactions; i++) {
        struct cs_network_transaction *curtrans = &(transactions->transactions[i]);
        int h = handle_udp_transaction(cfd, curtrans, c2s[0], s2c[1]);
        switch (h) {
            case 0:
                break;
            case 2:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(2);
            case 3:
                write(s2c[1], &_EXIT_PROCESS, U8SZ);
                exit(3);
        }
    }
    exit(0);
    PIPE_AND_FORK_MIDDLE();
    *cpid = pid;
    *client_in = c2s[1];
    *client_out = s2c[0];
    return 0;
    PIPE_AND_FORK_END();
}

