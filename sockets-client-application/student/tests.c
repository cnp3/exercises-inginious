// CTester template

#include <stdlib.h>
#include "student_code.h"
#include "CTester/CTester.h"

#define SETUP_FAILURES \
failures.socket = FAIL_ALWAYS; \
failures.socket_ret = 42; \
failures.connect = FAIL_ALWAYS; \
failures.connect_ret = 0; \
failures.send = FAIL_ALWAYS; \
failures.send_ret = -1; \
failures.recv = FAIL_ALWAYS; \
failures.recv_ret = -1;

#define SETUP_MONITORED \
monitored.socket = true; \
monitored.connect = true; \
monitored.send = true; \
monitored.recv = true;

#define UNUSED(v) ((void)v)

ssize_t static_recv_int(int sockfd, void *buf, size_t len, int flags) {
    UNUSED(flags);
    uint8_t resp[5] = {0xb4, 0, 0, 0, 10};
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(resp)) {
        return 0;
    }
    memcpy(buf, resp, sizeof(resp));
    return sizeof(resp);
}

ssize_t static_recv_str(int sockfd, void *buf, size_t len, int flags) {
    UNUSED(flags);
    uint8_t resp[3] = {0x8f, '1', '0'};
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(resp)) {
        return 0;
    }
    memcpy(buf, resp, sizeof(resp));
    return sizeof(resp);
}

ssize_t static_recv_str_9(int sockfd, void *buf, size_t len, int flags) {
    UNUSED(flags);
    uint8_t resp[2] = {0x8f, '9'};
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }
    if (!buf) {
        return 0;
    }
    if (len < sizeof(resp)) {
        return 0;
    }
    memcpy(buf, resp, sizeof(resp));
    return sizeof(resp);
}

void test_create_socket_socket_call() {
    set_test_metadata("client-app", _("Checks that the socket call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    SANDBOX_BEGIN;
    create_and_send_message(NULL, 0, NULL, 0, 0, 0, NULL);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.socket.last_params.domain, AF_INET6);
    CU_ASSERT_EQUAL(stats.socket.last_params.type, SOCK_DGRAM);
    CU_ASSERT_EQUAL(stats.socket.last_params.protocol, 0);
}

void test_create_socket_connect_call() {
    set_test_metadata("client-app", _("Checks that the connect call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
    create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(stats.connect.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_EQUAL(stats.connect.last_params.addr, addr_ptr);
    CU_ASSERT_EQUAL(stats.connect.last_params.addrlen, 42);
}

void test_create_socket_send_call() {
	set_test_metadata("client-app", _("Checks that the send call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

	void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
    create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(stats.send.called, 1);
    CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_EQUAL(stats.send.last_params.flags, 0);
}

void test_create_socket_send_content() {
	set_test_metadata("client-app", _("Checks that the correct content is send through the socket"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.send_ret = 1 + (4 * sizeof(int));

    {
        stats.send.last_params_buffered.buf = malloc(1024);
        stats.send.last_params_buffered.len = 1024;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        int ints_sent[4] = {htonl(1), htonl(2), htonl(3), htonl(4)};
        memset((void *)stats.send.last_params_buffered.buf, 0, 1024);
        stats.send.last_params_buffered.len = 1024;

        SANDBOX_BEGIN;
        create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.send.called, 1);
        CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.send.last_params.buf);
        if (stats.send.last_params.len != 1 + (4 * sizeof(int))) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Sum of ints: Expecting message of size %d, but sent message of size %d", (int) (1 + 4 * sizeof(int)), (int) stats.send.last_params.len);
            push_info_msg(buf);
        }

        uint8_t first_byte = *(uint8_t *)stats.send.last_params_buffered.buf;
        if ((first_byte & 0xE) >> 1 != 0) {
            CU_ASSERT(false);
            push_info_msg("Sum of ints: invalid opcode");
        }
        if ((first_byte & 0x1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Sum of ints: the F bit should not be set");
        }
        
        if (memcmp(&ints_sent, stats.send.last_params_buffered.buf + 1, stats.send.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Sum of ints: the content of the message is not correct");
        }
    }

    {
        free((void *)stats.send.last_params_buffered.buf);
        reinit_network_socket_stats();
        stats.send.last_params_buffered.buf = malloc(1024);
        stats.send.last_params_buffered.len = 1024;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        int ints_sent[4] = {htonl(1), htonl(2), htonl(3), htonl(4)};
        memset((void *)stats.send.last_params_buffered.buf, 0, 1024);
        stats.send.last_params_buffered.len = 1024;

        SANDBOX_BEGIN;
        create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.send.called, 1);
        CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.send.last_params.buf);
        if (stats.send.last_params.len != 1 + (4 * sizeof(int))) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Product of ints: Expecting message of size %d, but sent message of size %d", (int) (1 + 4 * sizeof(int)), (int) stats.send.last_params.len);
            push_info_msg(buf);
        }

        uint8_t first_byte = *(uint8_t *)stats.send.last_params_buffered.buf;
        if ((first_byte & 0xE) >> 1 != 1) {
            CU_ASSERT(false);
            push_info_msg("Product of ints: invalid opcode");
        }
        if ((first_byte & 0x1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Product of ints: the F bit should not be set");
        }

        if (memcmp(&ints_sent, stats.send.last_params_buffered.buf + 1, stats.send.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Product of ints: the content of the message is not correct");
        }
    }

    {
        free((void *)stats.send.last_params_buffered.buf);
        reinit_network_socket_stats();
        stats.send.last_params_buffered.buf = malloc(1024);
        stats.send.last_params_buffered.len = 1024;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        int ints_sent[4] = {htonl(1), htonl(2), htonl(3), htonl(4)};
        memset((void *)stats.send.last_params_buffered.buf, 0, 1024);
        stats.send.last_params_buffered.len = 1024;

        SANDBOX_BEGIN;
        create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.send.called, 1);
        CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.send.last_params.buf);
        if (stats.send.last_params.len != 1 + (4 * sizeof(int))) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Product of strs: Expecting message of size %d, but sent message of size %d", (int) (1 + 4 * sizeof(int)), (int) stats.send.last_params.len);
            push_info_msg(buf);
        }

        uint8_t first_byte = *(uint8_t *)stats.send.last_params_buffered.buf;
        if ((first_byte & 0xE) >> 1 != 1) {
            CU_ASSERT(false);
            push_info_msg("Product of strs: invalid opcode");
        }
        if ((first_byte & 0x1) != 1) {
            CU_ASSERT(false);
            push_info_msg("Product of strs: the F bit should be set");
        }

        if (memcmp(&ints_sent, stats.send.last_params_buffered.buf + 1, stats.send.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Product of strs: the content of the message is not correct");
        }
    }

    {
        free((void *)stats.send.last_params_buffered.buf);
        reinit_network_socket_stats();
        stats.send.last_params_buffered.buf = malloc(1024);
        stats.send.last_params_buffered.len = 1024;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        int ints_sent[4] = {htonl(1), htonl(2), htonl(3), htonl(4)};
        memset((void *)stats.send.last_params_buffered.buf, 0, 1024);
        stats.send.last_params_buffered.len = 1024;

        SANDBOX_BEGIN;
        create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.send.called, 1);
        CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.send.last_params.buf);
        if (stats.send.last_params.len != 1 + (4 * sizeof(int))) {
            CU_ASSERT(false);
            char buf[256];
            sprintf(buf, "Sum of strs: Expecting message of size %d, but sent message of size %d", (int) (1 + 4 * sizeof(int)), (int) stats.send.last_params.len);
            push_info_msg(buf);
        }

        uint8_t first_byte = *(uint8_t *)stats.send.last_params_buffered.buf;
        if ((first_byte & 0xE) >> 1 != 0) {
            CU_ASSERT(false);
            push_info_msg("Sum of strs: invalid opcode");
        }
        if ((first_byte & 0x1) != 1) {
            CU_ASSERT(false);
            push_info_msg("Sum of strs: the F bit should be set");
        }
        
        if (memcmp(&ints_sent, stats.send.last_params_buffered.buf + 1, stats.send.last_params.len - 1) != 0) {
            CU_ASSERT(false);
            push_info_msg("Sum of strs: the content of the message is not correct");
        }
    }
}

void test_create_socket_recv_call() {
    set_test_metadata("client-app", _("Checks that the recv call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.send_ret = 1 + (4 * sizeof(int));

    {
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_fun = (void(*)()) static_recv_int;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(stats.recv.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.recv.last_params.buf);
        CU_ASSERT(stats.recv.last_params.len > 4);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_ret = 1 + 2;
        failures.recv_fun = (void(*)()) static_recv_str;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(stats.recv.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.recv.last_params.buf);
        CU_ASSERT(stats.recv.last_params.len > 2);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_fun = (void(*)()) static_recv_int;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(stats.recv.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.recv.last_params.buf);
        CU_ASSERT(stats.recv.last_params.len > 4);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_fun = (void(*)()) static_recv_str;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(stats.recv.last_params.sockfd, stats.socket.last_return);
        CU_ASSERT_PTR_NOT_NULL(stats.recv.last_params.buf);
        CU_ASSERT(stats.recv.last_params.len > 2);
    }
}

void test_create_socket_return_content_call() {
    set_test_metadata("client-app", _("Checks that the function return value is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.send_ret = 1 + (4 * sizeof(int));

    {
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_fun = (void(*)()) static_recv_int;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(result, 10);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 1, 5};
        failures.recv_fun = (void(*)()) static_recv_str_9;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(result, 9);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 1, 5};
        failures.recv_fun = (void(*)()) static_recv_int;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 'd', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(result, 10);
    }

    {
        reinit_network_socket_stats();
        int ret;
        void *addr_ptr = (void *) 0xdeadbeef;
        int result = 0;
        int ints[4] = {1, 2, 3, 4};
        failures.recv_fun = (void(*)()) static_recv_str;

        SANDBOX_BEGIN;
        ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '*', 's', &result);
        SANDBOX_END;

        CU_ASSERT_EQUAL(stats.recv.called, 1);
        CU_ASSERT_EQUAL(ret, 0);
        CU_ASSERT_EQUAL(result, 10);
    }
}

void test_create_socket_failing_socket_call() {
	set_test_metadata("client-app", _("Checks that the socket call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.socket_ret = -1;
	
	int ret;
	void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
	ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_failing_connect_call() {
    set_test_metadata("client-app", _("Checks that the connect call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.connect_ret = -1;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
    ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_failing_send_call() {
	set_test_metadata("client-app", _("Checks that the send call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

	int ret;
	void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
    ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.send.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_failing_recv_call() {
    set_test_metadata("client-app", _("Checks that the recv call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.send_ret = 1 + (4 * sizeof(int));

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;
    int result = 0;
    int ints[4] = {1, 2, 3, 4};

    SANDBOX_BEGIN;
    ret = create_and_send_message(addr_ptr, 42, ints, sizeof(ints) / sizeof(int), '+', 'd', &result);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.recv.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

int main(int argc, char** argv)
{
    BAN_FUNCS();
    RUN(
            test_create_socket_socket_call,
            test_create_socket_connect_call,
            test_create_socket_send_call,
            test_create_socket_failing_socket_call,
            test_create_socket_failing_connect_call,
            test_create_socket_failing_send_call,
            test_create_socket_send_content,
            test_create_socket_recv_call,
            test_create_socket_failing_recv_call,
            test_create_socket_return_content_call
    );
}

