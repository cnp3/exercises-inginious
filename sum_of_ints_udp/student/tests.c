#include <stdlib.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "student_code.h"
#include "student_code_sol.h"
#include "CTester/CTester.h"
#include "CTester/read_write.h" // reinit_read_fd_table()
#include "CTester/util_sockets.h"
#include "CTester/util_inet.h"

bool recv_and_compare_udp(int sockfd, void *tab, size_t len)
{
    size_t received_bytes = 0, required_bytes = len, requested_bytes = required_bytes + 1, buflen = requested_bytes;
    ssize_t r = -1;
    void *buf = malloc(buflen);
    if (!buf)
        return false;
    r = recv(sockfd, buf, requested_bytes, 0);
    if (r < 0) {
        free(buf);
        return false;
    }
    received_bytes += r;
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


void test_client_udp_small_ints_symmetric()
{
    set_test_metadata("client_io_udp", _("Your UDP client can correctly send and receive a small number of integers."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 4, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x01010101, 0x02020202, 0x03030303, 0x04040404}, sum = htonl(0x0A0A0A0A);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    // Now, if we send on cfd, it will be recv'd on sfd, and if we send on sfd,
    // it will be recv'd on cfd: the data is waiting on both.
    // All we have to do is to write the answer on sfd.
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(rep, 0x0A0A0A0A);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare_udp(sfd, tabsol, tablen));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_no_int()
{
    set_test_metadata("client_io_udp", _("The UDP client works correctly with an empty vector"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 0, tablen = n*sizeof(uint32_t);
    uint32_t tab[0], sum = htonl(0);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(rep, 0);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare_udp(sfd, tabsol, tablen));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_htonl_send()
{
    set_test_metadata("client_io_udp", _("Your UDP client uses htonl when sending an integer"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x01020304, 0x04030201}, sum = htonl(0x05050505);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(rep, 0x05050505);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare_udp(sfd, tabsol, tablen));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_ntohl_recv()
{
    set_test_metadata("client_io_udp", _("The UDP client uses ntohl after having received an integer"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x80808080, 0x80808080}, sum = htonl(0x01010100);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(rep, 0x01010100);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare_udp(sfd, tabsol, tablen));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_no_sum()
{
    set_test_metadata("client_io_udp", _("The UDP client does not compute the sum itself"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x42424242, 0xBEBEBEBE}, sum = htonl(0x01020201);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(rep, 0x01020201);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_TRUE(recv_and_compare_udp(sfd, tabsol, tablen));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_too_much_ints()
{
    set_test_metadata("client_io_udp", _("The UDP client does not try to send messages that are too large"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 20000;
    uint32_t tab[n], sum = htonl(0x01020201);
    for (int i = 0; i < n; i++) {
        tab[i] = i;
    }
    uint32_t rep = 1;
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 0);
    CU_ASSERT_EQUAL(stats.send_all.called, 0);
    CU_ASSERT_EQUAL(r, -3);
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_failure_send()
{
    set_test_metadata("client_io_udp", _("The UDP client correctly handles the values returned by send"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2;
    uint32_t tab[] = {0x01020304, 0x05060708}, sum = htonl(0x06080A0B);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));
    failures.send = FAIL_ALWAYS;
    failures.send_ret = -1;
    failures.send_errno = ECONNRESET;
    failures.sendto = FAIL_ALWAYS;
    failures.sendto_ret = -1;
    failures.sendto_errno = ECONNRESET;
    failures.sendmsg = FAIL_ALWAYS;
    failures.sendmsg_ret = -1;
    failures.sendmsg_errno = ECONNRESET;
    failures.write = FAIL_ALWAYS;
    failures.write_ret = -1;
    failures.write_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 0);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.send.called) {
        CU_ASSERT_EQUAL(stats.send.last_params.len, sizeof(tab));
    } else if (stats.sendto.called) {
        CU_ASSERT_EQUAL(stats.sendto.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code neither used send nor sendto");
    }
    CU_ASSERT_EQUAL(r, -2);
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_client_udp_failure_recv()
{
    set_test_metadata("client_io_udp", _("The UDP client correctly handles the values returned by recv"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2;
    uint32_t tab[] = {0x01020304, 0x05060708}, sum = htonl(0x06080A0B);
    uint32_t tabsol[n], rep = 1;
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    int rr = connect_udp_server_to_client(sfd, cfd);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0 && rr == 0);
    CU_ASSERT_EQUAL(send(sfd, &sum, sizeof(sum), 0), sizeof(sum));
    failures.recv = FAIL_ALWAYS;
    failures.recv_ret = -1;
    failures.recv_errno = ECONNRESET;
    failures.recvfrom = FAIL_ALWAYS;
    failures.recvfrom_ret = -1;
    failures.recvfrom_errno = ECONNRESET;
    failures.recvmsg = FAIL_ALWAYS;
    failures.recvmsg_ret = -1;
    failures.recvmsg_errno = ECONNRESET;
    failures.read = FAIL_ALWAYS;
    failures.read_ret = -1;
    failures.read_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = get_sum_of_ints_udp(cfd, tab, n, &rep);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv_all.called, 1);
    CU_ASSERT_EQUAL(stats.send_all.called, 1);
    if (stats.recv.called) {
        CU_ASSERT_EQUAL(stats.recv.last_params.len, sizeof(uint32_t));
    } else if (stats.recvfrom.called) {
        CU_ASSERT_EQUAL(stats.recvfrom.last_params.len, sizeof(uint32_t));
    } else {
        CU_FAIL("Your code did not use recv or recvfrom ");
    }
    CU_ASSERT_EQUAL(r, -2);
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}



void test_server_udp_small_ints_symmetric()
{
    set_test_metadata("server_io_udp", _("The UDP server can correctly send and receive a small number of integers."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 4, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x01010101, 0x02020202, 0x03030303, 0x04040404}, sum = htonl(0x0A0A0A0A);
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_TRUE(recv_and_compare_udp(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_udp_no_int()
{
    set_test_metadata("server_io_udp", _("The UDP server correctly handles the reception of an empty vector"), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 0, tablen = n*sizeof(uint32_t);
    uint32_t tab[n], sum = htonl(0);
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_TRUE(recv_and_compare_udp(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_udp_ntohl_recv()
{
    set_test_metadata("server_io_udp", _("The UDP server uses ntohl when receiving the packet."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x01020304, 0x05060708}, sum = htonl(0x06080A0C);
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_TRUE(recv_and_compare_udp(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_udp_htonl_send()
{
    set_test_metadata("server_io_udp", _("The UDP server uses htonl when sending the answer."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x80808080, 0x80808080}, sum = htonl(0x01010100);
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_TRUE(recv_and_compare_udp(cfd, &sum, sizeof(sum)));
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_udp_failure_recv()
{
    set_test_metadata("server_io_udp", _("The UDP server correctly handles failures from recvfrom."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x80808080, 0x80808080};
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);
    failures.recvfrom = FAIL_FIRST;
    failures.recvfrom_ret = -1;
    failures.recvfrom_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, -2);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 0); // Should abort before this
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);
}

void test_server_udp_failure_send()
{
    set_test_metadata("server_io_udp", _("The UDP server correctly handles failures from sendto."), 1);
    reinit_network_socket_stats();
    reinit_network_inet_stats();
    reinit_all_monitored();
    int n = 2, tablen = n*sizeof(uint32_t);
    uint32_t tab[] = {0x80808080, 0x80808080};
    uint32_t tabsol[n];
    htonl_tab(tab, tabsol, n);
    int r = 0;
    MONITOR_ALL_RECV(monitored, true);
    MONITOR_ALL_SEND(monitored, true);
    monitored.read = true;
    monitored.write = true;
    MONITOR_ALL_BYTEORDER(monitored, true);
    int sfd = create_udp_server_socket("1618", AF_INET);
    int cfd = create_udp_client_socket(NULL, "1618", AF_INET);
    CU_ASSERT_TRUE(sfd >= 0 && cfd >= 0);
    CU_ASSERT_EQUAL(send(cfd, &tabsol, tablen, 0), tablen);
    failures.sendto = FAIL_FIRST;
    failures.sendto_ret = -1;
    failures.sendto_errno = ECONNRESET;

    SANDBOX_BEGIN;
    r = server_udp(sfd);
    SANDBOX_END;

    CU_ASSERT_EQUAL(r, -2);
    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.called, 1); // Should abort before this
    CU_ASSERT_EQUAL(close(cfd), 0);
    CU_ASSERT_EQUAL(close(sfd), 0);

    reinit_read_fd_table();
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
    BAN_FUNCS(recvmsg,sendmsg);
    reinit_read_fd_table();
    RUN(
            test_client_udp_small_ints_symmetric,
            test_client_udp_no_int,
            test_client_udp_htonl_send,
            test_client_udp_ntohl_recv,
            test_client_udp_no_sum,
            test_client_udp_too_much_ints,
            test_client_udp_failure_send,
            test_client_udp_failure_recv,
            test_server_udp_small_ints_symmetric,
            test_server_udp_no_int,
            test_server_udp_ntohl_recv,
            test_server_udp_htonl_send,
            test_server_udp_failure_recv,
            test_server_udp_failure_send
       );
}

