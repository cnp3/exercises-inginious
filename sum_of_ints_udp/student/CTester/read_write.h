/*
 * Functions and structure for manipulating read and write system calls,
 * as well as recv and send calls, in the presence of buffered data.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CTESTER_READ_WRITE_H__
#define __CTESTER_READ_WRITE_H__

#include <stddef.h>
#include <stdint.h>

/**
 * Functions and structures for manipulating read and write system calls,
 * as well as recv and send calls, in the presence of buffered data.
 *
 * An example use of these functions is with the recv system call on a socket.
 * In real-world situations, recv has a blocking and partial return
 * behaviour: when called, it blocks waiting for data to arrive (in general
 * through the network), and when the data has arrived, it returns all
 * the data readily available, and doesn't wait until more data arrives.
 *
 * In order to simulate that, we can provide recv with the information that
 * each read action should return up to a specified chunk of data, and should
 * therefore fragment the data returned to the caller.
 *
 * As we can use recv, recvfrom, recvmsg and read on a socket, all 4 system
 * calls need to be wrapped.
 *
 * Similarly, send can also partially send the provided data, if the underlying
 * send buffer and network doesn't have enough space to process the request.
 *
 * This partial-return behaviour of recv only happens on SOCK_STREAM sockets,
 * on which we generaly use the recv and send system calls (but we can still
 * use read, recvfrom and recvmsg without paying attention to flags or remote
 * address). On UDP (SOCK_DGRAM), this behaviour doesn't happen, in the sense
 * that it will try to return the whole datagram and discard the rest, and
 * the use of recvfrom (or recvmsg) is recommanded (recv is allowed but will
 * ignore the address). On SOCK_STREAM, send is recommanded (for sendto,
 * sendmsg, address should correspond to the connected address), and
 * on SOCK_DGRAM, sendto (and sendmsg) is recommanded (send or write should be
 * connected), and there is no valid partial send (a second send will create
 * a new datagram).
 *
 * On non-socket files, recv, recvfrom and recvmsg cannot be used, only read
 * is authorized. read may block if the file is a pipe or a fifo (these have
 * limited capacity). read also has a partial-return behaviour on pipes
 * and fifos.
 *
 * On non-socket files, send, sento and sendmsg cannot be used, only write is
 * authorized. write may block if the underlying device has no space available
 * at the moment (pipes and fifos), and may result in a partial-return.
 *
 * The functions listed here implement these blocking behaviour, on pipes
 * (which are remembered internally as pipes) and on sockets (which is a
 * subclass of pipes in some sense, with the addition of the recv and send
 * system calls) (which are remembered internally as sockets).
 */


/**
 * Enable globally the socket monitoring functionnalities. A value of true
 * activates it, a value of false deactivates it.
 * This means socket will add the returned fd to the list of sockets,
 * and subsequent recv and send will check that it is indeed a socket.
 * Additionnaly, read and write will also know that this is a pipe-like object.
 */
//int enable_socket_recv_send_monitoring(bool active);

/**
 * In addition to enable_socket_recv_send_monitoring, the bind, connect,
 * listen, accept and shutdown calls check whether it is a socket.
 */
//int enable_socket_all_monitoring(bool active);

/**
 * Enable globally the pipe monitoring functionnalities.
 * Currently it is not supported. Maybe in a future version...
 */
//int enable_pipe_monitoring(bool active);



//int set_recv_source(int fd, int sourcefd);

//int set_recv_buffer(int fd, void *buf, size_t buflen);

//int set_recv_policy(int fd, struct recv_policy_t *policy);
// Should define at some point the recv_policy_t structure.

/**
 * Structure representing a chunk of data, that is a fragment of the data
 * that is returned as a single entity by read/recv.
 * When simulating a partial-return read/recv, this is the fragments of data
 * that the caller receives sequentially (if it asks for the full fragment).
 * Fields:
 * - interval: time interval (in milliseconds) to wait before the chunk can be
 *   received. Should be no more than 1 thousand. Relative to the arrival
 *   of the previous chunk, and thus relative.
 * - buf : chunk of data to be read.
 * - buflen: length of this buffer.
 */
struct read_bufchunk_t {
    int interval;
    const void *buf;
    size_t buflen;
};

/**
 * The following macros define the modes to interpret the interval value:
 * - the first possibility (READ_WRITE_REAL_INTERVAL) tracks the time interval
 *   in "real time", which means that the student may not see the actual
 *   specified time interval if it waits long enough.
 * - the second possibility (READ_WRITE_AFTER_INTERVAL) only enforce the time
 *   interval between the end of a call that emptied a chunk and the actual
 *   read of the following call, which will start a new chunk. If the student
 *   waits enough, it may not see it.
 * - the third possibility (READ_WRITE_BEFORE_INTERVAL) imposes an actual wait
 *   each time the student calls recv and the chunk has not been read before
 *   (if it has, then it won't wait, but it will read only one chunk).
 *   The interval is enforced between the start of this call and the actual
 *   read of this same call. The student cannot ignore it.
 * Be careful to select the correct mode of operation,
 * otherwise it won't work like you want it.
 */
#define READ_WRITE_REAL_INTERVAL 1 // Real-time interval: if the student waits a lot, he won't see the interval
#define READ_WRITE_AFTER_INTERVAL 2 // At least interval µs after emptying the chunk
#define READ_WRITE_BEFORE_INTERVAL 3 // At least interval µs before reading a new chunk

/**
 * Structure representing a group of fragments of data, as it would be received
 * by subsequent read/recv of fragmented data. When simulating a partial-return
 * read/recv, this is the full data to be read, split into different chunks.
 */
struct read_buffer_t {
    int mode; // The mode of interpretation of interval
    size_t nchunks; // Number of chunks
    struct read_bufchunk_t *chunks; // Table of chunks
};

/**
 * Sets the data to be retrieved from read/recv, for partial-return,
 * when simulating fragmented arrival of data.
 * - fd: file descriptor.
 * - buf: the data to be received.
 * Returns
 * -  0 if fd was not previously set to have a read_buffer_t
 * -  1 if fd has been previously set to have a read_buffer_t
 * - -1 if malloc error
 * - -2 if argument error (typically buf->mode)
 */
int set_read_buffer(int fd, const struct read_buffer_t *buf);

/**
 * Returns the amount of bytes read from the previously provided buffer
 * for this file descriptor, or -1 if the file descriptor is not associated
 * with a read_buffer.
 */
ssize_t get_bytes_read(int fd);

/**
 * Returns the chunk id currently set up for reading of the previously provided
 * read buffer for this file descriptor, of -1 if there is no read_buffer
 * associated with this file descriptor.
 */
int get_current_chunk_id(int fd);

/**
 * Fills in buf with a read_buffer_t structure, from data (a continuous area
 * of memory), the list of offsets and the list of intervals. Allocates all
 * the structures needed.
 * Returns 0 if success, -1 in case of error (memory).
 */
int create_partial_read_buffer(void *data, size_t n, off_t *offsets, int *intervals, struct read_buffer_t *buf);

/**
 * Creates a read_buffer_t structure from data (a continuous area of memory),
 * the provided mode, the list of offsets and the list of intervals. Allocates
 * all the structures needed.
 * Returns the created buffer, or NULL in case of error.
 */
struct read_buffer_t *create_read_buffer(void *data, size_t n, off_t *offsets, int *intervals, int mode);

/**
 * Deallocates the provided read buffer. This doesn't free the actual data
 * returned by successive calls of recv and read, doesn't free buf itself
 * (so that a stack-allocated read_buffer_t can be passed on) and doesn't
 * reset the mode of the read_buffer_t. In short, it only frees up buf->chunks.
 * This is the inverse of create_partial_read_buffer.
 */
void free_partial_read_buffer(struct read_buffer_t *buf);

/**
 * Deallocates the provided read buffer. This doesn't free the actual data
 * returned by successive calls of recv and read. Also frees buf itself,
 * so no `free(buf)` is needed after this call.
 */
void free_read_buffer(struct read_buffer_t *buf);

/**
 * Removes all the association between fd's and read_buffer_t's that have been
 * previously set. It doesn't free the structures, however.
 */
void reinit_read_fd_table();


#endif // __CTESTER_READ_WRITE_H__

