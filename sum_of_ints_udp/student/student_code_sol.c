#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "student_code_sol.h"

#define MAX_SIZE 16000

int get_sum_of_ints_udp_sol(int sockfd, uint32_t *tab, size_t length, uint32_t *rep)
{
	// @@client_io_udp@start@@
	/*
	 * We assume that sockfd is an open socket,
	 * already set up to send packets to the server,
	 * and that we only have to send one message to the server,
	 * and receive its answer (one message too).
	 * Because it's UDP.
	 */
	if (length > MAX_SIZE) {
		fprintf(stderr, "Packet is too large to be sent over UDP");
		return -3;
	}
	ssize_t sent_bytes = 0, recv_bytes = 0;
	size_t total_bytes = length * sizeof(uint32_t);
	uint32_t *newtab = malloc(total_bytes);
	// TODO checker la taille du malloc pour vérifier qu'ils ont alloué suffisamment, et leur demander d'utiliser malloc.
	if (newtab == NULL) {
		perror("malloc");
		return -1;
	}
	for (unsigned int i = 0; i < length; i++) {
		newtab[i] = htonl(tab[i]);
	}
	sent_bytes = send(sockfd, newtab, total_bytes, 0);
	if (sent_bytes != (ssize_t) total_bytes) {
		if (sent_bytes == -1)
			perror("send");
		free(newtab);
		return -2;
	}
	free(newtab);
	uint32_t answer;
	recv_bytes = recv(sockfd, &answer, sizeof(uint32_t), 0);
	if (recv_bytes != sizeof(uint32_t)) {
		if (recv_bytes == -1)
			perror("recv");
		return -2;
	}
	*rep = ntohl(answer);
	return 0;
	// @@client_io_udp@end@@
}

int server_udp_sol(int sockfd)
{
	// @@server_io_udp@start@@
	/*
	 * Assume that sockfd is an open socket
	 * which is bound to the server's address,
	 * and (kinda) listening to messages,
	 * so that it can receive messages from clients.
	 * As such, all we have to do is just recvfrom the requests.
	 */
	struct sockaddr_storage client_addr;
	socklen_t client_addrlen = sizeof(client_addr);
	size_t bufsize = MAX_SIZE * sizeof(uint32_t);
	uint32_t *buf = malloc(bufsize);
	if (buf == NULL) {
		perror("malloc");
		return -1;
	}
	ssize_t r = recvfrom(sockfd, buf, bufsize, 0, (struct sockaddr*)&client_addr, &client_addrlen);
	if (r == -1) {
		perror("recvfrom");
		free(buf);
		return -2;
	}
	size_t recv_bytes = r;
	int nint = recv_bytes/sizeof(uint32_t);
	uint32_t sum = 0;
	for (int i = 0; i < nint; i++) {
		sum += ntohl(buf[i]);
	}
	free(buf);
	sum = htonl(sum);
	ssize_t s = sendto(sockfd, (void*)&sum, sizeof(uint32_t), 0, (struct sockaddr*)&client_addr, client_addrlen);
	if (s != sizeof(uint32_t)) {
		perror("sendto");
		return -2;
	}
	return 0;
	// @@server_io_udp@end@@
}

