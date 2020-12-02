// CTester template

#include <stdlib.h>
#include "student_code.h"
#include "CTester/CTester.h"

#define SETUP_FAILURES \
failures.socket = FAIL_ALWAYS; \
failures.socket_ret = 42; \
failures.bind = FAIL_ALWAYS; \
failures.bind_ret = 0; \
failures.recvfrom = FAIL_ALWAYS; \
failures.recvfrom_ret = -1; \
failures.sendto = FAIL_ALWAYS; \
failures.sendto_ret = 4; \

#define SETUP_MONITORED \
monitored.socket = true; \
monitored.bind = true; \
monitored.recvfrom = true; \
monitored.sendto = true;

#define UNUSED(v) ((void)v)

ssize_t static_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    int numbers[5] = {htonl(1), htonl(2), htonl(3), htonl(4), htonl(5)};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(numbers)) {
        return 0;
    }
    memcpy(buf, numbers, sizeof(numbers));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(numbers);
}

void test_create_socket_socket_call() {
    set_test_metadata("sending-strings", _("Checks that the socket call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    SANDBOX_BEGIN;
    recv_and_handle_message(NULL, 0);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.socket.last_params.domain, AF_INET6);
    CU_ASSERT_EQUAL(stats.socket.last_params.type, SOCK_DGRAM);
    CU_ASSERT_EQUAL(stats.socket.last_params.protocol, 0);
}

void test_create_socket_failing_socket_call() {
    set_test_metadata("sending-strings", _("Checks that the socket call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.socket_ret = -1;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    ret = recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_bind_call() {
    set_test_metadata("sending-strings", _("Checks that the bind call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.bind.called, 1);
    CU_ASSERT_EQUAL(stats.bind.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_EQUAL(stats.bind.last_params.addr, addr_ptr);
    CU_ASSERT_EQUAL(stats.bind.last_params.addrlen, 42);
}

void test_create_socket_failing_bind_call() {
    set_test_metadata("sending-strings", _("Checks that the bind call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.bind_ret = -1;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    ret = recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.bind.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_recvfrom_call() {
    set_test_metadata("sending-strings", _("Checks that the recvfrom call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom;

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.recvfrom.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_PTR_NOT_NULL(stats.recvfrom.last_params.src_addr);
    CU_ASSERT_PTR_NOT_NULL(stats.recvfrom.last_params.addrlen);
    CU_ASSERT_EQUAL(stats.recvfrom.last_return, 5 * sizeof(int));
}

void test_create_socket_failing_recvfrom_call() {
    set_test_metadata("sending-strings", _("Checks that the recvfrom call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    ret = recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_sendto_call() {
    set_test_metadata("sending-strings", _("Checks that the sendto call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom;
    stats.sendto.last_params_buffered.buf = malloc(1024);
    stats.sendto.last_params_buffered.dest_addr = malloc(1024);

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_EQUAL(stats.sendto.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_PTR_NOT_NULL(stats.sendto.last_params.buf);
    CU_ASSERT_PTR_NOT_NULL(stats.sendto.last_params.dest_addr);
    CU_ASSERT_EQUAL(stats.sendto.last_params.addrlen, 1);
    CU_ASSERT_EQUAL(*(char *)stats.sendto.last_params_buffered.dest_addr, 42);
}

void test_create_socket_sent_content_call() {
    set_test_metadata("sending-strings", _("Checks that the content sent is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom;
    stats.sendto.last_params_buffered.buf = malloc(1024);
    stats.sendto.last_params_buffered.dest_addr = malloc(1024);

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    int sum = 1 + 2 + 3 + 4 + 5;
    char buf[10];
    int buf_len = sprintf(buf, "%d", sum);
    CU_ASSERT_EQUAL(stats.sendto.last_params.len, buf_len);
    CU_ASSERT_EQUAL(memcmp(&buf, stats.sendto.last_params_buffered.buf, stats.sendto.last_params.len), 0);
}

void test_create_socket_failing_sendto_call() {
    set_test_metadata("sending-strings", _("Checks that the sendto call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom;
    failures.sendto_ret = -1;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    ret = recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.sendto.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}


int main(int argc, char** argv)
{
    BAN_FUNCS();
    RUN(
            test_create_socket_socket_call,
            test_create_socket_failing_socket_call,
            test_create_socket_bind_call,
            test_create_socket_failing_bind_call,
            test_create_socket_recvfrom_call,
            test_create_socket_failing_recvfrom_call,
            test_create_socket_sendto_call,
            test_create_socket_failing_sendto_call,
            test_create_socket_sent_content_call
    );
}

