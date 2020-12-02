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
failures.send_ret = -1;

#define SETUP_MONITORED \
monitored.socket = true; \
monitored.connect = true; \
monitored.send = true;

void test_create_socket_socket_call() {
    set_test_metadata("create-socket", _("Checks that the socket call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    SANDBOX_BEGIN;
    create_and_send_message(NULL, 0);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.socket.last_params.domain, AF_INET6);
    CU_ASSERT_EQUAL(stats.socket.last_params.type, SOCK_DGRAM);
    CU_ASSERT_EQUAL(stats.socket.last_params.protocol, 0);
}

void test_create_socket_connect_call() {
    set_test_metadata("create-socket", _("Checks that the connect call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(stats.connect.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_EQUAL(stats.connect.last_params.addr, addr_ptr);
    CU_ASSERT_EQUAL(stats.connect.last_params.addrlen, 42);
}

void test_create_socket_send_call() {
	set_test_metadata("create-socket", _("Checks that the send call is correct"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

	void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(stats.send.called, 1);
    CU_ASSERT_EQUAL(stats.send.last_params.sockfd, stats.socket.last_return);
    CU_ASSERT_EQUAL(stats.send.last_params.flags, 0);
}

void test_create_socket_send_content() {
	set_test_metadata("create-socket", _("Checks that the correct content is send through the socket"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    stats.send.last_params_buffered.buf = malloc(1024);
    stats.send.last_params_buffered.len = 1024;

    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(stats.connect.last_params.sockfd, 42);
    CU_ASSERT_EQUAL(stats.send.called, 1);
    CU_ASSERT_EQUAL(stats.send.last_params.sockfd, 42);
    CU_ASSERT_PTR_NOT_NULL(stats.send.last_params.buf);
    CU_ASSERT_EQUAL(stats.send.last_params.len, 5 * sizeof(int));
    CU_ASSERT_EQUAL(stats.send.last_params.flags, 0);

    int odds[5] = {htonl(1), htonl(3), htonl(5), htonl(7), htonl(9)};
    CU_ASSERT_EQUAL(memcmp(&odds, stats.send.last_params_buffered.buf, stats.send.last_params.len), 0);
}

void test_create_socket_failing_socket_call() {
	set_test_metadata("create-socket", _("Checks that the socket call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.socket_ret = -1;
	
	int ret;
	void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
	ret = create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.socket.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_failing_connect_call() {
    set_test_metadata("create-socket", _("Checks that the connect call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;
    failures.connect_ret = -1;

    int ret;
    void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
    ret = create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.connect.called, 1);
    CU_ASSERT_EQUAL(ret, -1);
}

void test_create_socket_failing_send_call() {
	set_test_metadata("create-socket", _("Checks that the send call failure is handled correctly"), 1);

    SETUP_FAILURES;
    SETUP_MONITORED;

	int ret;
	void *addr_ptr = (void *) 0xdeadbeef;

    SANDBOX_BEGIN;
	ret = create_and_send_message(addr_ptr, 42);
    SANDBOX_END;

    CU_ASSERT_EQUAL(stats.send.called, 1);
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
            test_create_socket_send_content
    );
}

