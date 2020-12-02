#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
 * This function should return -1 if something went wrong.
 */

int recv_and_handle_message(const struct sockaddr *src_addr, socklen_t addrlen);
