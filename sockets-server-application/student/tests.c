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

void dump_ptr(void *ptr, size_t ptr_len) {
    uint8_t *data = (uint8_t *) ptr;
    for (uint i = 0; i < ptr_len; i++) {
        printf("0x%02x ", data[i]);
    }
    printf("\n");
}

ssize_t static_recvfrom_sum_int(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    uint8_t message[1 + (3 * sizeof(int))] = {0x50, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 3};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(message)) {
        return 0;
    }
    memcpy(buf, message, sizeof(message));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(message);
}

ssize_t static_recvfrom_sum_str(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    uint8_t message[1 + (3 * sizeof(int))] = {0x71, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 3};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(message)) {
        return 0;
    }
    memcpy(buf, message, sizeof(message));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(message);
}

ssize_t static_recvfrom_mul_int(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    uint8_t message[1 + (3 * sizeof(int))] = {0x52, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 3};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(message)) {
        return 0;
    }
    memcpy(buf, message, sizeof(message));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(message);
}

ssize_t static_recvfrom_mul_str(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    uint8_t message[1 + (3 * sizeof(int))] = {0x83, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 3};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(message)) {
        return 0;
    }
    memcpy(buf, message, sizeof(message));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(message);
}

ssize_t static_recvfrom_unknown_op(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    UNUSED(flags);
    uint8_t message[1 + (3 * sizeof(int))] = {0x45, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 3};
    if (sockfd < 0) {
        errno = ENOTSOCK;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(message)) {
        return 0;
    }
    memcpy(buf, message, sizeof(message));
    if (src_addr) {
        *(char *)src_addr = 42;
        *addrlen = 1;
    }
    return sizeof(message);
}

void test_create_socket_socket_call() {
    set_test_metadata("server-app", _("Checks that the socket call is correct"), 1);

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
    set_test_metadata("server-app", _("Checks that the socket call failure is handled correctly"), 1);

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
    set_test_metadata("server-app", _("Checks that the bind call is correct"), 1);

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
    set_test_metadata("server-app", _("Checks that the bind call failure is handled correctly"), 1);

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
    set_test_metadata("server-app", _("Checks that the recvfrom call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom_sum_int;

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    recv_and_handle_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recvfrom.called, 1);
    CU_ASSERT_EQUAL(stats.recvfrom.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_PTR_NOT_NULL(stats.recvfrom.last_params.src_addr);
    CU_ASSERT_PTR_NOT_NULL(stats.recvfrom.last_params.addrlen);
}

void test_create_socket_failing_recvfrom_call() {
    set_test_metadata("server-app", _("Checks that the recvfrom call failure is handled correctly"), 1);

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
    set_test_metadata("server-app", _("Checks that the sendto call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom_sum_int;
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
    set_test_metadata("server-app", _("Checks that the content sent is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    {
        reinit_network_socket_stats();
        failures.recvfrom_fun = (void (*)()) static_recvfrom_sum_int;
        stats.sendto.last_params_buffered.buf = malloc(1024);
        stats.sendto.last_params_buffered.dest_addr = malloc(1024);

        void *addr_ptr = (void *) 0xdeadbeef;

        SANDBOX_BEGIN;
        recv_and_handle_message(addr_ptr, 42);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.sendto.called, 1);
        if (((*(uint8_t*)stats.sendto.last_params_buffered.buf) & 0x80) == 0) {
            CU_ASSERT(false);
            push_info_msg("The S bit is not set for a valid request");
        }
        uint8_t response_payload[sizeof(int)] = {0, 0, 0, 9};
        if (stats.sendto.last_params.len != 1 + sizeof(response_payload)) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Sum of ints: Expecting message of size %d, but received message of size %d", (int) (1 + sizeof(response_payload)), (int) stats.sendto.last_params.len);
            push_info_msg(buf);
        }
        if (memcmp(response_payload, stats.sendto.last_params_buffered.buf + 1, stats.sendto.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("The sum of ints does not return the correct result");
        }
    }

    {
        reinit_network_socket_stats();
        failures.recvfrom_fun = (void (*)()) static_recvfrom_sum_str;
        stats.sendto.last_params_buffered.buf = malloc(1024);
        stats.sendto.last_params_buffered.dest_addr = malloc(1024);

        void *addr_ptr = (void *) 0xdeadbeef;

        SANDBOX_BEGIN;
        recv_and_handle_message(addr_ptr, 42);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.sendto.called, 1);
        if (((*(uint8_t*)stats.sendto.last_params_buffered.buf) & 0x80) == 0) {
            CU_ASSERT(false);
            push_info_msg("The S bit is not set for a valid request");
        }
        char response_payload[1] = {'9'};
        if (stats.sendto.last_params.len != 1 + sizeof(response_payload)) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Sum of strs: Expecting message of size %d, but received message of size %d", (int) (1 + sizeof(response_payload)), (int) stats.sendto.last_params.len);
        }
        if (memcmp(response_payload, stats.sendto.last_params_buffered.buf + 1, stats.sendto.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("The sum of strs does not return the correct result");
        }
    }

    {
        reinit_network_socket_stats();
        failures.recvfrom_fun = (void (*)()) static_recvfrom_mul_int;
        stats.sendto.last_params_buffered.buf = malloc(1024);
        stats.sendto.last_params_buffered.dest_addr = malloc(1024);

        void *addr_ptr = (void *) 0xdeadbeef;

        SANDBOX_BEGIN;
        recv_and_handle_message(addr_ptr, 42);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.sendto.called, 1);
        if (((*(uint8_t*)stats.sendto.last_params_buffered.buf) & 0x80) == 0) {
            CU_ASSERT(false);
            push_info_msg("The S bit is not set for a valid request");
        }
        uint8_t response_payload[sizeof(int)] = {0, 0, 0, 24};
        if (stats.sendto.last_params.len != 1 + sizeof(response_payload)) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Product of ints: Expecting message of size %d, but received message of size %d", (int) (1 + sizeof(response_payload)), (int) stats.sendto.last_params.len);
        }
        if (memcmp(response_payload, stats.sendto.last_params_buffered.buf + 1, stats.sendto.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("The product of ints does not return the correct result");
        }
    }

    {
        reinit_network_socket_stats();
        failures.recvfrom_fun = (void (*)()) static_recvfrom_mul_str;
        stats.sendto.last_params_buffered.buf = malloc(1024);
        stats.sendto.last_params_buffered.dest_addr = malloc(1024);

        void *addr_ptr = (void *) 0xdeadbeef;

        SANDBOX_BEGIN;
        recv_and_handle_message(addr_ptr, 42);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.sendto.called, 1);
        if (((*(uint8_t*)stats.sendto.last_params_buffered.buf) & 0x80) == 0) {
            CU_ASSERT(false);
            push_info_msg("The S bit is not set for a valid request");
        }
        char response_payload[2] = {'2', '4'};
        if (stats.sendto.last_params.len != 1 + sizeof(response_payload)) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Product of strs: Expecting message of size %d, but received message of size %d", (int) (1 + sizeof(response_payload)), (int) stats.sendto.last_params.len);
        }
        if (memcmp(response_payload, stats.sendto.last_params_buffered.buf + 1, stats.sendto.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("The product of strs does not return the correct result");
        }
    }

    {
        reinit_network_socket_stats();
        failures.recvfrom_fun = (void (*)()) static_recvfrom_unknown_op;
        stats.sendto.last_params_buffered.buf = malloc(1024);
        stats.sendto.last_params_buffered.dest_addr = malloc(1024);

        void *addr_ptr = (void *) 0xdeadbeef;

        SANDBOX_BEGIN;
        recv_and_handle_message(addr_ptr, 42);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.sendto.called, 1);
        CU_ASSERT(stats.sendto.last_params.len >= 1);
        if (((*(uint8_t*)stats.sendto.last_params_buffered.buf) & 0x80) != 0) {
            CU_ASSERT(false);
            push_info_msg("The S bit is set for an invalid request");
        }
    }
}

void test_create_socket_failing_sendto_call() {
    set_test_metadata("server-app", _("Checks that the sendto call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.recvfrom_fun = (void(*)()) static_recvfrom_sum_str;
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

