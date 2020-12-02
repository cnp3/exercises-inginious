#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "read_write.h"

int64_t MILLION = 1000*1000;
#define BILLION (1000*1000*1000)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))


/**
 * Auxiliary structures and functions
 */

void getnanotime(struct timespec *res)
{
    clock_gettime(CLOCK_REALTIME, res);
}

int64_t get_time_interval(const struct timespec *pasttime, const struct timespec *curtime)
{
    int64_t past = pasttime->tv_sec;
    past *= BILLION;
    past += pasttime->tv_nsec;
    int64_t cur = curtime->tv_sec;
    cur *= BILLION;
    cur += curtime->tv_nsec;
    int64_t interval = cur - past;
    if (interval < 0) {
        fprintf(stderr, "BUG: negative time interval: %lld %lld %lld", (long long)past, (long long)cur, (long long)interval); fflush(stderr);
    }
    return interval;
}


/**
 * read-related
 */

// #define READ_MODE_BUFCHUK 1 // not used

/**
 * Precision for the 'interval' field.
 * It contains the wait time for the current chunk, as measured from the previous call.
 * It may be positive: in this case, it means the next data was not available
 * at the end of the previous call, and it is the time the current call has
 * to wait before accessing data. Minus the interval between the two calls.
 * It may be negative: in this case, it represents the opposite of the amout
 * of time the current chunk has been available.
 * Thanks to this field, we can enable the chunks at more or less the right moment.
 */
struct read_item {
    int fd; // the file descriptor this structure applies to
    //int mode; // the type of data provider used; not used
    const struct read_buffer_t *buf; // Provided read_buffer_t structure
    unsigned int chunk_id; // Current chunk, or next chunk to be received
    size_t bytes_read; // Number of bytes read inside the current chunk
    struct timespec last_time; // Time of the end of the last call of read on this fd/socket
    int64_t interval; // In real-time mode (READ_WRITE_REAL_INTERVAL), real wait interval for the current chunk (in nanoseconds).
};

struct read_fd_table_t {
    size_t n;
    struct read_item *items;
} read_fd_table;

struct read_item *read_get_entry(int fd)
{
    for (unsigned int i = 0; i < read_fd_table.n; i++) {
        if (read_fd_table.items[i].fd == fd)
            return &(read_fd_table.items[i]);
    }
    return NULL;
}

bool fd_is_read_buffered(int fd)
{
    return (read_get_entry(fd) != NULL);
}

void reinit_read_fd_table_item(int i)
{
    // We need to clean up _recv_fd_table.items[i]
    read_fd_table.items[i].buf = NULL; // We're not responsible to free it.
    read_fd_table.items[i].chunk_id = 0;
    read_fd_table.items[i].interval = 0;
    read_fd_table.items[i].last_time = (struct timespec) {
        .tv_sec = 0,
        .tv_nsec = 0
    };
}


int read_remove_entry(int fd)
{
    bool found = false;
    for (unsigned int i = 0; i < read_fd_table.n; i++) {
        if (read_fd_table.items[i].fd == fd) {
            found = true;
            if (i < read_fd_table.n - 1) {
                struct read_item *dest = &(read_fd_table.items[i]);
                memmove(dest, dest + 1, (read_fd_table.n - i - 1) * sizeof(struct read_item));
                memset(&(read_fd_table.items[read_fd_table.n-1]), 0, sizeof(struct read_item));
                // This is not really a problem that we have a useless thing at the end
            }
            read_fd_table.n--;
        }
    }
    if (read_fd_table.n == 0) {
        free(read_fd_table.items);
        read_fd_table.items = NULL;
    }
    return (found ? 1 : 0);
}

/**
 * Returns a pointer to an entry in the table,
 * where we can safely store our informations.
 * May be a pre-existing chunk,
 * in which case the fd's will match.
 */
struct read_item *read_get_new_entry(int fd)
{
    unsigned int i = 0;
    for (i = 0; i < read_fd_table.n; i++) {
        if (read_fd_table.items[i].fd == fd) {
            // We should clean it before changing it
            reinit_read_fd_table_item(i);
            break;
        }
    }
    if (i >= read_fd_table.n) {
        // realloc
        struct read_item *tmp = realloc(read_fd_table.items, (read_fd_table.n + 1) * sizeof(struct read_item));
        if (tmp == NULL)
            return NULL;
        read_fd_table.items = tmp;
        read_fd_table.n++;
    }
    // Now, we can insert at index i
    return &(read_fd_table.items[i]);
}

size_t read_handle_buffer_no_rt(struct read_item *cur, void *buf, size_t len, int flags, int64_t call_interval)
{
    const struct read_bufchunk_t *curchunk = &(cur->buf->chunks[cur->chunk_id]);
    if (cur->bytes_read == 0) {
        // We may have to wait
        int64_t sleeptime = 0;
        if (cur->buf->mode == READ_WRITE_BEFORE_INTERVAL) {
            sleeptime = MILLION * (curchunk->interval);
        } else { // READ_WRITE_AFTER_INTERVAL
            sleeptime = MILLION * (curchunk->interval) - call_interval;
        }
        if (sleeptime > 0) {
            if ((flags & MSG_DONTWAIT) != 0) {
                errno = EAGAIN;
                return -1;
            }
            struct timespec tmp = (struct timespec) {
                .tv_sec = sleeptime / BILLION,
                .tv_nsec = sleeptime % BILLION
            };
            nanosleep(&tmp, NULL);
            getnanotime(&(cur->last_time)); // We need to update
        }
    }
    size_t bytes_left = curchunk->buflen - cur->bytes_read;
    size_t transfered_bytes = MIN(len, bytes_left);
    memmove(buf, (curchunk->buf + cur->bytes_read), transfered_bytes);
    cur->bytes_read += transfered_bytes;
    if (cur->bytes_read >= curchunk->buflen) {
        cur->chunk_id++;
        cur->bytes_read = 0;
    }
    return transfered_bytes;
}

size_t read_handle_buffer_rt(struct read_item *cur, void *buf, size_t len, int flags, int64_t call_interval)
{
    /*
     * Assume the following:
     * - cur->interval was the time to wait for the current chunk to become
     *   active, at the end of the previous call.
     * - cur->bytes_read is 0 only if we have to wait.
     */
    cur->interval -= call_interval;
    /*
     * Assume the following:
     * - cur->interval is the remaining time to wait for the current chunk to become active.
     * - cur->bytes_read is 0 only if we have to wait.
     */
    if (cur->interval > 0) {
        if ((flags & MSG_DONTWAIT) != 0) {
            // The call would block but the caller requested it shouldn't block
            errno = EAGAIN;
            return -1;
        }
        int64_t sleeptime = cur->interval;
        struct timespec tmp = (struct timespec) {
            .tv_sec = sleeptime / BILLION,
            .tv_nsec = sleeptime % BILLION
        };
        nanosleep(&tmp, NULL);
        cur->interval = 0;
        getnanotime(&(cur->last_time));
    }
    /*
     * Assume the following:
     * - cur->interval is the negative of the time the current chunk has been available.
     */
    size_t total_bytes = 0;
    for (unsigned int i = cur->chunk_id; i < cur->buf->nchunks; i++) {
        if (len <= 0) {
            break;
        }
        const struct read_bufchunk_t *curchunk = &(cur->buf->chunks[i]);
        size_t bytes_left = curchunk->buflen - cur->bytes_read;
        size_t transfered_bytes = MIN(len, bytes_left);
        memmove((buf + total_bytes), (curchunk->buf + cur->bytes_read), transfered_bytes);
        cur->bytes_read += transfered_bytes;
        len -= transfered_bytes;
        total_bytes += transfered_bytes;
        if (cur->bytes_read >= curchunk->buflen) {
            // Emptied chunk
            cur->chunk_id++;
            cur->bytes_read = 0;
            if (cur->chunk_id < cur->buf->nchunks) {
                // Update interval
                cur->interval += MILLION * (cur->buf->chunks[cur->chunk_id].interval);
                if (cur->interval > 0) {
                    break; // Not available yet
                }
            }
        }
    }
    return total_bytes;
}

ssize_t read_handle_buffer(int fd, void *buf, size_t len, int flags)
{
    // TODO add support for MSG_OOB, MSG_PEEK and MSG_WAITALL to flags
    struct read_item *cur = read_get_entry(fd);
    if (cur == NULL) {
        errno = EINTR; // This is about the only case where this can happen
        return -1;
    }
    struct timespec curtime;
    getnanotime(&curtime);
    int64_t call_interval = get_time_interval(&(cur->last_time), &curtime);
    cur->last_time = curtime;
    if (cur->chunk_id >= cur->buf->nchunks) {
        // Nothing left to read; we can return immediately 0
        // TODO remove this as a particular case
        return 0;
    }
    size_t bytes_transfered = 0;
    if (cur->buf->mode == READ_WRITE_BEFORE_INTERVAL || cur->buf->mode == READ_WRITE_AFTER_INTERVAL) {
        bytes_transfered = read_handle_buffer_no_rt(cur, buf, len, flags, call_interval);
    } else if (cur->buf->mode == READ_WRITE_REAL_INTERVAL) {
        bytes_transfered = read_handle_buffer_rt(cur, buf, len, flags, call_interval);
    } else {
        return -1;
    }
    return bytes_transfered;
}

void reinit_read_fd_table()
{
    // As we're not responsible to clean up all the recv_buffer_t, we can just free everything up
    free(read_fd_table.items);
    read_fd_table.items = NULL;
    read_fd_table.n = 0;
}

/*int enable_socket_recv_send_monitoring(bool active)
{
    // TODO
}*/

/*int enable_socket_all_monitoring(bool active)
{
    // TODO
}*/

/*
int enable_pipe_monitoring(bool active)
{
    // TODO
}
*/


int set_read_buffer(int fd, const struct read_buffer_t *buf)
{
    if (buf == NULL) {
        return read_remove_entry(fd);
    }
    if (buf->mode != READ_WRITE_REAL_INTERVAL &&
        buf->mode != READ_WRITE_AFTER_INTERVAL &&
        buf->mode != READ_WRITE_BEFORE_INTERVAL) {
        return -2;
    }
    bool already_there = false;
    if (fd_is_read_buffered(fd)) {
        already_there = true;
    }
    struct read_item *tmp = read_get_new_entry(fd);
    if (tmp == NULL) {
        return -1;
    }
    tmp->fd = fd;
    tmp->buf = buf;
    tmp->chunk_id = 0;
    tmp->bytes_read = 0;
    getnanotime(&(tmp->last_time));
    if (buf->mode == READ_WRITE_REAL_INTERVAL && buf->nchunks > 0) {
        // The first wait interval should be that of the first chunk.
        tmp->interval = MILLION * (buf->chunks[0].interval);
    } else {
        tmp->interval = 0;
    }
    return (already_there ? 1 : 0);
}

ssize_t get_bytes_read(int fd)
{
    //return -1;
    struct read_item *cur = read_get_entry(fd);
    if (cur == NULL)
        return -1;
    size_t bread = 0;
    for (unsigned int i = 0; i < cur->chunk_id; i++) {
        bread += cur->buf->chunks[i].buflen;
    }
    if (cur->chunk_id < cur->buf->nchunks) {
        bread += cur->bytes_read;
    }
    return bread;
}

int get_current_chunk_id(int fd)
{
    return -1;
    struct read_item *cur = read_get_entry(fd);
    if (cur == NULL)
        return -1;
    return cur->chunk_id;
}

int create_partial_read_buffer(void *data, size_t n, off_t *offsets, int *intervals, struct read_buffer_t *buf)
{
    buf->nchunks = n;
    buf->chunks = malloc(n * sizeof(struct read_bufchunk_t));
    if (!(buf->chunks)) {
        free(buf);
        return -1;
    }
    off_t cum_offset = 0;
    for (unsigned int i = 0; i < n; i++) {
        buf->chunks[i] = (struct read_bufchunk_t) {
            .buf = (data + cum_offset),
            .buflen = offsets[i],
            .interval = intervals[i]
        };
        cum_offset += offsets[i];
    }
    return 0;
}

struct read_buffer_t *create_read_buffer(void *data, size_t n, off_t *offsets, int *intervals, int mode)
{
    struct read_buffer_t *buf = malloc(sizeof(*buf));
    if (!buf)
        return NULL;
    buf->mode = mode;
    if (create_partial_read_buffer(data, n, offsets, intervals, buf)) {
        free(buf);
        return NULL;
    }
    return buf;
}

void free_partial_read_buffer(struct read_buffer_t *buf)
{
    for (unsigned int i = 0; i < buf->nchunks; i++) {
        buf->chunks[i] = (struct read_bufchunk_t) {
            .buf = NULL,
            .buflen = 0,
            .interval = 0
        };
    }
    free(buf->chunks);
    buf->chunks = NULL;
    buf->nchunks = 0;
}

void free_read_buffer(struct read_buffer_t *buf)
{
    free_partial_read_buffer(buf);
    buf->mode = 0;
    free(buf);
}

