#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "student_code.h"

#define MAX_SIZE 16000

int get_sum_of_ints_tcp(int sockfd, uint32_t *tab, size_t length, uint32_t *rep)
{
	@@client_io_tcp@@
}

int server_tcp(int sockfd)
{
	@@server_io_tcp@@
}

