/**
 * @file util_sockets.h
 * Utility functions and structure for creating and manipulating sockets,
 * for creating and launching test servers and clients.
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

#ifndef __CTESTER_UTIL_SOCKETS_H__
#define __CTESTER_UTIL_SOCKETS_H__

#include <stdint.h>

/**
 * Constants used when reporting the output of the mock server or client.
 * Currenly, they may be combined with any of them;
 * maybe we should exclude some combination.
 */
// Self conflicting constants
#define OK 0
#define TOO_MUCH 1 // And the amount should follow
#define TOO_FEW 2 // And the amount should follow
#define RECV_ERROR 3 // Followed by value of errno.
#define NOTHING_RECV 4
// Left : 6-15
// Or-ing constants
#define SEND_ERROR 8 // To indicate the real type of error
#define NOT_SAME_DATA 16 // Idealy, the recv'd tab should follow, preceded by its length, but not yet implemented
#define NOT_SAME_ADDR 32 // The recv'd address should follow, preceded by its length (socklen_t)
#define EXIT_PROCESS 64 // The child process exits: beware of SIGPIPE!
#define EXTEND_MSG 128 // Next octet gives the status

#define U8SZ (sizeof(uint8_t))
#define U16SZ (sizeof(uint16_t))

/**
 * Creates a socket with the specified domain, type, a nought protocol,
 * and which is either bound to the specified host name (which should be NULL)
 * and service name (if do_bind is true and flags has AI_PASSIVE) or which is
 * connected to the specified host name and service name (if do_bind is false).
 * The call to getaddrinfo has flags AI_NUMERICHOST and AI_NUMERICSERV
 * set, in order to disable DNS; additionnal flags are then passed with it.
 * If do_bind, it is important to note that the socket will NOT listen
 * for incoming connections: it is up to the caller to activate it.
 */
int create_socket(const char *host, const char *serv, int domain, int type, int flags, int do_bind);

/**
 * Creates a socket with the specified domain (AF_INET or AF_INET6),
 * type (either SOCK_STREAM or SOCK_SEQPACKET), a nought protocol,
 * and which is listening for incoming connections on port specified in serv.
 * This calls create_socket, and thus requires serv to be numeric.
 */
int create_tcp_server_socket(const char *serv, int domain, int type);

/**
 * Creates a socket with the specified domain (AF_INET or AF_INET6),
 * type (either SOCK_STREAM or SOCK_SEQPACKET), a nought protocol,
 * and connected to the specified address and port (host and serv).
 * This calls create_socket, and thus requires host and serv to be numeric.
 */
int create_tcp_client_socket(const char *host, const char *serv, int domain, int type);

/**
 * Creates a UDP socket with the specified domain (AF_INET or AF_INET6),
 * a nought protocol, and which accepts UDP packets on port specified in serv.
 * This calls create_socket, and thus requires serv to be numeric.
 */
int create_udp_server_socket(const char *serv, int domain);

/**
 * Creates a UDP socket with the specified domain (AF_INET or AF_INET6), a zero
 * protocol, and connected to the specified address and port (host and serv).
 * This calls create_socket, and thus requires host and serv to be numeric.
 */
int create_udp_client_socket(const char *host, const char *serv, int domain);

/**
 * Connects the provided sfd UDP server socket to the address of the provided
 * cfd client socket, such that calls to send on sfd will correctly send
 * to the client and will be receivable from cfd. cfd should be "connected"
 * to the server (i.e., its default address should be that of the server).
 * Returns 0 in case of success, or -1 if there was an error.
 */
int connect_udp_server_to_client(int sfd, int cfd);

#define RECV_CHUNK 1
#define SEND_CHUNK 2

/**
 * A chunk of data to be recv or send by the server/client.
 * type can be
 * - RECV_CHUNK, in which case the server/client will try to receive it
 *   and will compare the received bytes with the provided data
 *   (no byte order conversion are done!)
 * - SEND_CHUNK, in which case the server/client will send to the remote end
 *   the data, and will report any transmission error.
 * Warning, the chunks should have the network byte order, as the server/client
 * will not try to reverse the bytes.
 */
struct cs_network_chunk {
    void *data;
    size_t data_length;
    int type;
};

/**
 * A transaction, which is a list of chunks exchanged between the server/client
 * and the remote end.
 * A transaction is successful if all RECV_CHUNK chunks have been received
 * correctly, and if the server/client hasn't been asked to quit.
 */
struct cs_network_transaction {
    struct cs_network_chunk *chunks;
    size_t nchunks;
    struct sockaddr *addr;
    socklen_t addrlen;
};

/**
 * A list of transactions to be tested one after another, using one instance
 * of the server/client.
 */
struct cs_network_transactions {
    struct cs_network_transaction *transactions;
    size_t ntransactions;
};

/**
 * Launches a TCP-listening and accepting server, on port specified by serv,
 * in a separate process. In order to communicate with the caller,
 * server_in is filled with the writing end of a pipe transfering toward
 * the server (for example, stop condition), server_out is filled with
 * the reading end of a pipe written by the server with the results of
 * the requested transactions, and spid is filled with the process pid,
 * so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 * The server exits with code 3 if it receives something on server_in,
 * with code 2 if there was an error, and with 1 if it couldn't create the socket.
 */
int launch_test_tcp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid);

/**
 * Launches a TCP client, connecting and sending to host host on port serv,
 * in a separate process. To communicate with the caller, client_in is filled
 * with the writing end of a pipe transfering toward the client (for example,
 * stop condition), client_out is filled with the reading end of a pipe written
 * by the client with the results of the requested transactions, and cpid
 * is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the client, 1 otherwise.
 */
int launch_test_tcp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid);

/**
 * Launches a UDP server, accepting messages on port specified by serv,
 * in a separate process. To communicate with the caller, server_in is filled
 * with the writing end of a pipe transfering toward the server (for example,
 * stop condition), server_out is filled with the reading end of a pipe
 * written by the server with the results of the requested transactions,
 * and spid is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 */
int launch_test_udp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid);

/**
 * Launches a UDP client, sending messages to host host on port serv,
 * in a separate process. To communicate with the caller, client_in is filled
 * with the writing end of a pipe transfering toward the client (for example,
 * stop condition), client_out is filled with the reading end of a pipe written
 * by the client with the results of the requested transactions, and cpid
 * is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 */
int launch_test_udp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid);

#endif // __CTESTER_UTIL_SOCKETS_H__

