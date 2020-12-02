#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "student_code_sol.h"

#define MAX_SIZE 16000

int get_sum_of_ints_tcp_sol(int sockfd, uint32_t *tab, size_t length, uint32_t *rep)
{
    // @@client_io_tcp@start@@
    ssize_t s, sent_bytes = 0, recv_bytes = 0, total_size = (length + 1) * sizeof(uint32_t);
    uint32_t *newtab = malloc(total_size);
    if (newtab == NULL) {
        perror("malloc");
        return -1;
    }
    newtab[0] = htonl(length);
    for (unsigned int i = 0; i < length; i++) {
        newtab[i + 1] = htonl(tab[i]);
    }
    do {
        s = send(sockfd, ((void*)(newtab) + sent_bytes), total_size - sent_bytes, 0);
        if (s == -1) {
            perror("send");
            free(newtab);
            return -2;
        }
        sent_bytes += s;
    } while (sent_bytes != total_size);
    free(newtab);
    // Done with sending, now waiting for an answer.
    uint32_t answer;
    do {
        s = recv(sockfd, ((void*)(&answer) + recv_bytes), (sizeof(uint32_t) - recv_bytes), 0);
        if (s == -1) {
            perror("recv");
            return -2;
        }
        recv_bytes += s;
    } while (recv_bytes != sizeof(uint32_t));
    *rep = ntohl(answer);
    return 0;
    // @@client_io_tcp@end@@
}

int server_tcp_sol(int sockfd)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen;
    int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (clientfd == -1) {
        perror("accept");
        return -2;
    }

    ssize_t r;
    size_t length = 0, recv_bytes = 0;
    do {
        r = recv(clientfd, ((void*)(&length) + recv_bytes), sizeof(uint32_t) - recv_bytes, 0);
        if (r == -1) {
            perror("recv");
            close(clientfd);
            return -2;
        }
        recv_bytes += r;
    } while (recv_bytes != sizeof(uint32_t));
    length = ntohl(length);
    // length now contains the number of 32-bits integers to read.

    size_t bufsize = length * sizeof(uint32_t);
    uint32_t *buf = malloc(bufsize);
    if (buf == NULL) {
        close(clientfd);
        perror("malloc");
        return -1;
    }

    recv_bytes = 0;
    do {
        r = recv(clientfd, ((void*)(buf) + recv_bytes), (bufsize - recv_bytes), 0);
        if (r == -1) {
            perror("recv");
            close(clientfd);
            free(buf);
            return -2;
        }
        recv_bytes += r;
    } while (recv_bytes != bufsize);

    uint32_t sum = 0;
    for (unsigned i = 0; i < length; i++) {
        sum += ntohl(buf[i]);
    }
    free(buf);
    sum = htonl(sum);

    ssize_t s = 0;
    size_t sent_bytes = 0;
    do {
        s = send(clientfd, ((void*)(&sum) + sent_bytes), sizeof(uint32_t) - sent_bytes, 0);
        if (s == -1) {
            perror("send");
            close(clientfd);
            return -2;
        }
        sent_bytes += s;
    } while (sent_bytes != sizeof(uint32_t));

    if (close(clientfd) == -1) {
        perror("close");
        return -2;
    }
    return 0;
}


