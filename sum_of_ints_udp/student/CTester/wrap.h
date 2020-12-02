#ifndef __WRAP_H_
#define __WRAP_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

// headers for all stats

#include "wrap_getpid.h"
#include "wrap_file.h"
#include "wrap_malloc.h"
#include "wrap_mutex.h"
#include "wrap_network_dns.h"
#include "wrap_network_socket.h"
#include "wrap_network_inet.h"
#include "wrap_sleep.h"

// Basic structures for system call wrapper
// verifies whether the system call needs to be monitored. Each
// supported system call must be referenced here

// flag must be set to true if the system call must be monitored

struct wrap_monitor_t {
  bool getpid;
  bool open;
  bool creat;
  bool close;
  bool read;
  bool write;
  bool stat;
  bool fstat;
  bool lseek;
  bool free;
  bool malloc;
  bool calloc;
  bool realloc;
  bool pthread_mutex_lock;
  bool pthread_mutex_trylock;
  bool pthread_mutex_unlock;
  bool pthread_mutex_init;
  bool pthread_mutex_destroy;
  bool sleep;

  bool getaddrinfo;
  bool getnameinfo;
  bool freeaddrinfo;
  bool gai_strerror;

  bool accept;
  bool bind;
  bool connect;
  bool listen;
  bool poll;
  bool recv;
  bool recvfrom;
  bool recvmsg;
  bool select;
  bool send;
  bool sendto;
  bool sendmsg;
  bool shutdown;
  bool socket;

  bool htons;
  bool ntohs;
  bool htonl;
  bool ntohl;
};

#define MONITOR_ALL_RECV(m, v) do { \
  m.recv = m.recvfrom = m.recvmsg = v; \
} while (0);

#define MONITOR_ALL_SEND(m, v) do { \
  m.send = m.sendto = m.sendmsg = v; \
} while (0);

#define MONITOR_ALL_BYTEORDER(m, v) do { \
  m.ntohs = m.htons = m.ntohl = m.htonl = v; \
} while (0);

#define MAX_LOG 1000

// log for specific system calls
struct malloc_t {
    int n;
    struct malloc_elem_t log[MAX_LOG];
};

struct wrap_log_t {
  struct malloc_t malloc;
} ;


// failures are defined as a bitmask. If set to zero, the system call
// never fails. Otherwise, the fail information is shifted to the right
// and the next system call will fail if its low order bit is set
// we assume that there are no more than 32 system calls that need
// fail. This should be sufficient for most inginious exercices.
// If not, the idea can be extended to support more bits.

#define FAIL_ALWAYS 0b11111111111111111111111111111111
#define FAIL_FIRST  0b00000000000000000000000000000001 
#define FAIL_TWICE  0b00000000000000000000000000000011 
#define FAIL_NEVER  0b00000000000000000000000000000000
#define FAIL_SECOND 0b00000000000000000000000000000010 
#define FAIL_THIRD  0b00000000000000000000000000000100

#define FAIL(v) (((v & 0b00000000000000000000000000000001) == 0b00000000000000000000000000000001) )
#define NEXT(v) (v==FAIL_ALWAYS ? FAIL_ALWAYS : v >> 1)
struct wrap_fail_t {
  // bool getpid - this call cannot fail
  uint32_t open;  // indicates whether next open will fail
  int open_ret;   // return value if open fails
  int open_errno; // errno value set if open fails

  uint32_t creat;
  int creat_ret;
  int creat_errno;

  uint32_t close;
  int close_ret;
  int close_errno;

  uint32_t read;
  int read_ret;
  int read_errno;

  uint32_t write;
  int write_ret;
  int write_errno;

  uint32_t stat;
  int stat_ret;
  int stat_errno;

  uint32_t fstat;
  int fstat_ret;
  int fstat_errno;

  uint32_t lseek;
  int lseek_ret;
  int lseek_errno;

  uint32_t malloc;
  void *malloc_ret;

  uint32_t calloc;
  void *calloc_ret;
  
  uint32_t realloc;
  void *realloc_ret;

  uint32_t free;

  uint32_t pthread_mutex_lock;
  int pthread_mutex_lock_ret;
  int pthread_mutex_lock_errno;

  uint32_t pthread_mutex_trylock;
  int pthread_mutex_trylock_ret;
  int pthread_mutex_trylock_errno;

  uint32_t pthread_mutex_unlock;
  int pthread_mutex_unlock_ret;
  int pthread_mutex_unlock_errno;

  uint32_t pthread_mutex_init;
  int pthread_mutex_init_ret;
  int pthread_mutex_init_errno;

  uint32_t pthread_mutex_destroy;
  int pthread_mutex_destroy_ret;
  int pthread_mutex_destroy_errno;

  uint32_t sleep;
  unsigned int sleep_ret;

  uint32_t getaddrinfo;
  int getaddrinfo_ret;
  int getaddrinfo_errno;

  uint32_t getnameinfo;
  int getnameinfo_ret;
  int getnameinfo_errno;

  // freeaddrinfo - this call cannot fail, unless res is not a valid pointer.

  // gai_strerror - this call cannot fail;
  // if its parameter is an unkown error code, it just returns "Unknown error code"

  uint32_t accept;
  int accept_ret;
  int accept_errno;

  uint32_t bind;
  int bind_ret;
  int bind_errno;

  uint32_t connect;
  int connect_ret;
  int connect_errno;

  uint32_t listen;
  int listen_ret;
  int listen_errno;

  uint32_t poll;
  int poll_ret;
  int poll_errno;

  uint32_t recv;
  int recv_ret;
  int recv_errno;

  uint32_t recvfrom;
  int recvfrom_ret;
  int recvfrom_errno;

  uint32_t recvmsg;
  int recvmsg_ret;
  int recvmsg_errno;

  uint32_t select;
  int select_ret;
  int select_errno;

  uint32_t send;
  int send_ret;
  int send_errno;

  uint32_t sendto;
  int sendto_ret;
  int sendto_errno;

  uint32_t sendmsg;
  int sendmsg_ret;
  int sendmsg_errno;

  uint32_t shutdown;
  int shutdown_ret;
  int shutdown_errno;

  uint32_t socket;
  int socket_ret;
  int socket_errno;

  // byte-order functions (htonl...) cannot fail
} ;


struct wrap_stats_t {
  struct stats_getpid_t getpid;
  struct stats_open_t open;
  struct stats_creat_t creat;
  struct stats_close_t close;
  struct stats_read_t read;
  struct stats_write_t write;
  struct stats_stat_t stat;
  struct stats_fstat_t fstat;
  struct stats_lseek_t lseek;
  struct stats_malloc_t malloc;
  struct stats_calloc_t calloc;
  struct stats_memory_t memory;
  struct stats_free_t free;
  struct stats_realloc_t realloc;
  struct stats_pthread_mutex_lock_t pthread_mutex_lock;
  struct stats_pthread_mutex_trylock_t pthread_mutex_trylock;
  struct stats_pthread_mutex_unlock_t pthread_mutex_unlock;
  struct stats_pthread_mutex_unlock_t pthread_mutex_init;
  struct stats_pthread_mutex_unlock_t pthread_mutex_destroy;
  struct stats_sleep_t sleep;

  struct stats_getaddrinfo_t getaddrinfo;
  struct stats_getnameinfo_t getnameinfo;
  struct stats_freeaddrinfo_t freeaddrinfo;
  struct stats_gai_strerror_t gai_strerror;

  struct stats_accept_t accept;
  struct stats_bind_t bind;
  struct stats_connect_t connect;
  struct stats_listen_t listen;
  struct stats_poll_t poll;
  struct stats_recv_t recv;
  struct stats_recvfrom_t recvfrom;
  struct stats_recvmsg_t recvmsg;
  struct stats_recv_all_t recv_all;
  struct stats_select_t select;
  struct stats_send_t send;
  struct stats_sendto_t sendto;
  struct stats_sendmsg_t sendmsg;
  struct stats_send_all_t send_all;
  struct stats_shutdown_t shutdown;
  struct stats_socket_t socket;

  struct stats_htons_t htons;
  struct stats_ntohs_t ntohs;
  struct stats_htonl_t htonl;
  struct stats_ntohl_t ntohl;
};

#endif // __WRAP_H_
