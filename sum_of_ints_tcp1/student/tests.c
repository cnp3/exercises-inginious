#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "student_code.h"
#include "student_code_sol.h"
#include "CTester/CTester.h"
#include "CTester/read_write.h"
#include "CTester/util_sockets.h"
#include "CTester/util_inet.h"

struct read_item {
    int fd;
    const struct read_buffer_t *buf;
    unsigned int chunk_id;
    size_t bytes_read;
    struct timespec last_time;
    int64_t interval;
};

struct read_fd_table_t {
    size_t n;
    struct read_item *items;
};

extern struct read_fd_table_t read_fd_table;

int intervals0[] = {0};
off_t offsets0[] = {sizeof(uint32_t)};

bool recv_and_compare(int sockfd, void *tab, size_t len)
{
    size_t received_bytes = 0, required_bytes = len, requested_bytes = required_bytes + 1, buflen = requested_bytes;
    ssize_t r = -1;
    void *buf = malloc(buflen);
    if (!buf)
        return false;
    do {
        r = recv(sockfd, (buf + received_bytes), requested_bytes - received_bytes, 0);
        if (r < 0) {
            free(buf);
            return false;
        }
        received_bytes += r;
    } while (received_bytes < requested_bytes && r != 0);
    /*
     * received_bytes >= requested_bytes || r == 0 :
     * - received_bytes > requested_bytes: too much
     * - received_bytes == requested_bytes && r == 0: ok
     * - received_bytes == requested_bytes && r != 0: we should check one more;
     *   not done here
     */
    if (received_bytes > required_bytes) {
        // Too much data sent
        free(buf);
        return false;
    }
    if (received_bytes < required_bytes) {
        // Too few data sent
        free(buf);
        return false;
    }
    int c = memcmp(buf, tab, required_bytes);
    free(buf);
    return (c == 0);
}



void test_client_tcp_small_ints_symmetric()
{
    set_test_metadata("client_io_tcp", _("Your TCP client can correctly send and receive a vector containing a small number of integers."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4, tablen = (n+1)*sizeof(uint32_t);
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404}, sum = htonl(0x0A0A0A0A);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x0A0A0A0A);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_no_int()
{
    set_test_metadata("client_io_tcp", _("Your TCP client can correctly handle an empty vector."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 0, tablen = (0+1)*sizeof(uint32_t);
    uint32_t tab[0+1], sum = htonl(0);
    uint32_t tabsol[0+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_one_int()
{
    set_test_metadata("client_io_tcp", _("Your TCP client can correctly handle a vector containing a single element"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 1, tablen = (1+1)*sizeof(uint32_t);
    uint32_t tab[] = {1, 0x42424242}, sum = htonl(0x42424242);
    uint32_t tabsol[1+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x42424242);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_no_sum()
{
    set_test_metadata("client_io_tcp", _("The TCP client doesn't do the sum itself."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 2, tablen = (2+1)*sizeof(uint32_t);
    uint32_t tab[] = {2, 0x42424242, 0xBEBEBEBE}, sum = htonl(0x01020201);
    uint32_t tabsol[2+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x01020201);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_small_ints_htonl_send()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly uses htonl when sending"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 2, tablen = (2+1)*sizeof(uint32_t);
    uint32_t tab[] = {2, 0x01020304, 0x04030201}, sum = htonl(0x05050505);
    uint32_t tabsol[2+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x05050505);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_small_ints_ntohl_recv()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly uses ntohl when receiving data"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 2, tablen = (2+1)*sizeof(uint32_t);
    uint32_t tab[] = {2, 0x80808080, 0x80808080}, sum = htonl(0x01010100);
    uint32_t tabsol[2+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x01010100);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_htonl_ntohl()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly uses htonl and ntohl"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4, tablen = (n+1)*sizeof(uint32_t);
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x1C202428);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_large_ints()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly handles large vectors"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 16000, tablen = (16000+1)*sizeof(uint32_t);
    uint32_t tab[16000+1], sum = 0, sumsum = 0;
    tab[0] = n;
    for (int i = 0; i < n; i++) {
        tab[i+1] = i;
        sumsum += i;
    }
    sum = htonl(sumsum);
    uint32_t tabsol[16000+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 1, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, sumsum);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_recv_waiting()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly handles the fact that a recv system call can return only a few bytes at a time"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4, tablen = (n+1)*sizeof(uint32_t);
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    off_t offsets[] = {1, 2, sizeof(uint32_t)-3}; // Normally this is 1, 2, 1
    int intervals[] = {40, 20, 40};
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 3, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(rep, 0x1C202428);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(scfd, tabsol, tablen));
    CU_ASSERT_EQUAL(get_bytes_read(cfd), sizeof(sum));

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_failure_send()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly checks the return value of the send system call"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 3, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);
    failures.send = FAIL_FIRST;
    failures.send_ret = -1;
    failures.send_errno = ECONNRESET;
    failures.sendto = FAIL_FIRST;
    failures.sendto_ret = -1;
    failures.sendto_errno = ECONNRESET;
    failures.sendmsg = FAIL_FIRST;
    failures.sendmsg_ret = -1;
    failures.sendmsg_errno = ECONNRESET;
    failures.write = FAIL_FIRST;
    failures.write_ret = -1;
    failures.write_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);
    free_partial_read_buffer(&rbuf);

    CU_ASSERT_EQUAL(r, -2);

    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_failure_recv_first()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly handles failures (ECONNREFUSED) of the recv system call"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    off_t offsets[] = {1, 2, sizeof(uint32_t)-3};
    int intervals[] = {40, 20, 40};
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 3, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);
    failures.recv = FAIL_FIRST;
    failures.recv_ret = -1;
    failures.recv_errno = ECONNREFUSED;
    failures.recvfrom = FAIL_FIRST;
    failures.recvfrom_ret = -1;
    failures.recvfrom_errno = ECONNREFUSED;
    failures.recvmsg = FAIL_FIRST;
    failures.recvmsg_ret = -1;
    failures.recvmsg_errno = ECONNREFUSED;
    failures.read = FAIL_FIRST;
    failures.read_ret = -1;
    failures.read_errno = ECONNREFUSED;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);

    CU_ASSERT_EQUAL(r, -2);
    CU_ASSERT_EQUAL(get_bytes_read(cfd), 0);

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_tcp_failure_recv_second()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly handles failures (EINTR) of the recv system call"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    off_t offsets[] = {1, 2, sizeof(uint32_t)-3};
    int intervals[] = {40, 20, 40};
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 3, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);
    failures.recv = FAIL_SECOND;
    failures.recv_ret = -1;
    failures.recv_errno = EINTR;
    failures.recvfrom = FAIL_SECOND;
    failures.recvfrom_ret = -1;
    failures.recvfrom_errno = EINTR;
    failures.recvmsg = FAIL_SECOND;
    failures.recvmsg_ret = -1;
    failures.recvmsg_errno = EINTR;
    failures.read = FAIL_SECOND;
    failures.read_ret = -1;
    failures.read_errno = EINTR;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(get_bytes_read(cfd), 1); // As only the second one should fail

    CU_ASSERT_EQUAL(r, -2);

    free_partial_read_buffer(&rbuf);
    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

// This test is not absolutely necessary, but it's always good to have
void test_client_tcp_failure_malloc()
{
    set_test_metadata("client_io_tcp", _("Your TCP client correctly handles failures from malloc"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[4+1], rep = 1;
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    monitored.malloc = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    CU_ASSERT_EQUAL(create_partial_read_buffer(&sum, 3, offsets0, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(cfd, &rbuf), 0);
    failures.malloc = FAIL_FIRST;
    failures.malloc_ret = NULL;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_tcp(cfd, tab+1, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(close(cfd), 0);
    free_partial_read_buffer(&rbuf);

    CU_ASSERT_EQUAL(r, -1);

    CU_ASSERT_EQUAL(close(scfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}



void test_server_tcp_small_ints_symmetric()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly processes a vector containing a small number of elements"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404}, sum = htonl(0x0A0A0A0A);
    uint32_t tabsol[4+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_close()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly closes the TCP connection that it has accepted."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404}, sum = htonl(0x0A0A0A0A);
    uint32_t tabsol[4+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    ssize_t rrs = 0, rrr = 0;
    int errnosend = 0, errnorecv = 0, rrc = 0, errnoclose = 0;
    struct sigaction newact, oldact;
    newact.sa_handler = SIG_IGN;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    CU_ASSERT_EQUAL(sigaction(SIGPIPE, &newact, &oldact), 0);
    errno = 0;
    rrs = send(scfd, &r, sizeof(r), 0);
    errnosend = errno;
    rrr = recv(scfd, &r, sizeof(r), MSG_DONTWAIT);
    errnorecv = errno;
    CU_ASSERT_EQUAL(sigaction(SIGPIPE, &oldact, NULL), 0);
    CU_ASSERT_EQUAL(rrs, -1);
    CU_ASSERT_EQUAL(rrr, -1);
    CU_ASSERT_TRUE(errnosend == ENOTCONN || errnosend == EPIPE || errnosend == EBADF);
    CU_ASSERT_TRUE(errnorecv == ENOTCONN || errnorecv == EPIPE || errnorecv == EBADF);
    CU_ASSERT_TRUE(errnorecv != EAGAIN && errnorecv != EWOULDBLOCK);
    rrc = close(scfd);
    errnoclose = errno;
    CU_ASSERT_EQUAL(rrc, -1);
    CU_ASSERT_TRUE(errnoclose == EBADF);

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
    reinit_read_fd_table(); // As it is the last test, we should free the read_item of the read_fd_table.
}

void test_server_tcp_no_int()
{
    set_test_metadata("server_io_tcp", _("Your TCP server works correctly if a client initiates a TCP connection but closes it without sending data"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 0;
    uint32_t tab[1] = {0}, sum = htonl(0);
    uint32_t tabsol[n+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_ntohl_recv()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly uses ntohl after having received data on the TCP connection"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 3;
    uint32_t tab[] = {3, 0x01020304, 0x05060708, 0x090A0B0C}, sum = htonl(0X0F121518);
    uint32_t tabsol[n+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_htonl_send()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly uses htonl when sending its answer"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 2;
    uint32_t tab[] = {2, 0x80808080, 0x80808080}, sum = htonl(0X01010100);
    uint32_t tabsol[n+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_large_ints()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly operates when it receives a large vector of integers"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 16000;
    uint32_t tab[n+1], sum = 0;
    tab[0] = n;
    for (int i = 0; i < 16000; i++) {
        tab[i+1] = i;
        sum += i;
    }
    sum = htonl(sum);
    uint32_t tabsol[n+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_recv_waiting()
{
    set_test_metadata("server_io_tcp", _("Your TCP server works correctly when the recv system call only returns a few bytes at a time"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01020304, 0x05060708, 0x090A0B0C, 0x0D0E0F10}, sum = htonl(0x1C202428);
    uint32_t tabsol[n+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {3, 6, sizeof(tabsol)-9};
    int intervals[] = {40, 20, 40};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 3, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);
    fprintf(stderr, "AA %d %d %d\n", sfd, cfd, scfd);

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));
    fprintf(stderr, "Lol %u %u %u\n", (unsigned)get_bytes_read(scfd), (unsigned)sizeof(tabsol), (unsigned)get_current_chunk_id(scfd));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_failure_send()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly handles errors returned by send"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404};
    uint32_t tabsol[4+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {sizeof(tabsol)};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals0, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);
    failures.send = FAIL_FIRST;
    failures.send_ret = -1;
    failures.send_errno = ECONNRESET;
    failures.sendto = FAIL_FIRST;
    failures.sendto_ret = -1;
    failures.sendto_errno = ECONNRESET;
    failures.sendmsg = FAIL_FIRST;
    failures.sendmsg_ret = -1;
    failures.sendmsg_errno = ECONNRESET;
    failures.write = FAIL_FIRST;
    failures.write_ret = -1;
    failures.write_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, -2);
    int rr = close(scfd);
    CU_ASSERT_TRUE(rr == -1 && errno == EBADF);
    CU_ASSERT_EQUAL(get_bytes_read(scfd), sizeof(tabsol));

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_failure_recv()
{
    set_test_metadata("server_io_tcp", _("Your TCP sender correctly handles the errors returned by recv"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404};
    uint32_t tabsol[4+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {10, sizeof(tabsol)-10};
    int intervals[] = {40, 40};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 1, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);
    failures.recv = FAIL_SECOND;
    failures.recv_ret = -1;
    failures.recv_errno = ECONNRESET;
    failures.recvfrom = FAIL_SECOND;
    failures.recvfrom_ret = -1;
    failures.recvfrom_errno = ECONNRESET;
    failures.recvmsg = FAIL_SECOND;
    failures.recvmsg_ret = -1;
    failures.recvmsg_errno = ECONNRESET;
    failures.read = FAIL_FIRST;
    failures.read_ret = -1;
    failures.read_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, -2);
    int rr = close(scfd);
    CU_ASSERT_TRUE(rr == -1 && errno == EBADF);
    CU_ASSERT_TRUE(1 <= get_bytes_read(scfd) && get_bytes_read(scfd) <= 10);

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_tcp_failure_malloc()
{
    set_test_metadata("server_io_tcp", _("Your TCP server correctly handles the errors returned by malloc "), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_read_fd_table();
    reinit_all_monitored();
    int n = 4;
    uint32_t tab[] = {4, 0x01010101, 0x02020202, 0x03030303, 0x04040404};
    uint32_t tabsol[4+1];
    htonl_tab(tab, tabsol, n+1);
    int r = 0;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    MONITOR_ALL_RECV(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_tcp_server_socket("1618", AF_INET, SOCK_STREAM);
    int cfd = create_tcp_client_socket(NULL, "1618", AF_INET, SOCK_STREAM);
    int scfd = accept(sfd, (struct sockaddr*)&addr, &addrlen);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && scfd >= 0);
    CU_ASSERT_EQUAL(addrlen, sizeof(struct sockaddr_in));
    CU_ASSERT_EQUAL(((struct sockaddr_in*)&addr)->sin_addr.s_addr, htonl(INADDR_LOOPBACK));
    monitored.accept = true;
    failures.accept = FAIL_FIRST; // Quick and dirty way to circumvent a system call.
    failures.accept_ret = scfd;
    failures.accept_errno = 0;
    struct read_buffer_t rbuf;
    rbuf.mode = READ_WRITE_BEFORE_INTERVAL;
    off_t offsets[] = {10, sizeof(tabsol)-10};
    int intervals[] = {40, 40};
    CU_ASSERT_EQUAL(create_partial_read_buffer(tabsol, 2, offsets, intervals, &rbuf), 0);
    CU_ASSERT_EQUAL(set_read_buffer(scfd, &rbuf), 0);
    monitored.malloc = true;
    failures.malloc = FAIL_FIRST;
    failures.malloc_ret = NULL;

    SANDBOX_BEGIN;
    r = server_tcp(sfd);
    SANDBOX_END;

    close(scfd); // Needed for recv_and_compare

    CU_ASSERT_EQUAL(r, -1);
    int rr = close(scfd);
    CU_ASSERT_TRUE(rr == -1 && errno == EBADF);

    free_partial_read_buffer(&rbuf); // NEVER free a read_buffer before we've finished using it ;-)
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}



/**
 * Empty test, to cleanup the fd table and have a clean valgrind check
 */
void test_end()
{
    reinit_read_fd_table();
}



int main(int argc,char** argv)
{
    BAN_FUNCS();
    RUN(
            test_client_tcp_small_ints_symmetric,
            test_client_tcp_no_int,
            test_client_tcp_one_int,
            test_client_tcp_small_ints_htonl_send,
            test_client_tcp_small_ints_ntohl_recv,
            test_client_tcp_no_sum,
            test_client_tcp_large_ints,
            test_client_tcp_htonl_ntohl,
            test_client_tcp_recv_waiting,
            test_client_tcp_failure_send,
            test_client_tcp_failure_recv_first,
            test_client_tcp_failure_recv_second,
            test_client_tcp_failure_malloc,
            test_server_tcp_small_ints_symmetric,
            test_server_tcp_close,
            test_server_tcp_no_int,
            test_server_tcp_ntohl_recv,
            test_server_tcp_htonl_send,
            test_server_tcp_large_ints,
            test_server_tcp_recv_waiting,
            test_server_tcp_failure_send,
            test_server_tcp_failure_recv,
            test_server_tcp_failure_malloc
       );
}

