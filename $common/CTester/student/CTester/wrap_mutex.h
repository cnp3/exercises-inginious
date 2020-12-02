/**
 * @file wrap_mutex.h
 * Wrapper for the POSIX thread mutex functions.
 */
/*
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
#ifndef __WRAP_MUTEX_H_
#define __WRAP_MUTEX_H_

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

// never remove statistics from this structure, they could be
// used by existing exercices. You might add some additional information
// if it can help to validate some exercices

struct stats_pthread_mutex_lock_t {
  int called;           // number of times the system call has been called
  pid_t last_return;    // last return value
  pthread_mutex_t *last_arg; // last mutex passed as argument

};

void init_pthread_mutex_lock();
void clean_pthread_mutex_lock();
void resetstats_pthread_mutex_lock();

struct stats_pthread_mutex_trylock_t {
  int called;           // number of times the system call has been called
  pid_t last_return;    // last return value
  pthread_mutex_t *last_arg; // last mutex passed as argument
};

void init_pthread_mutex_trylock();
void clean_pthread_mutex_trylock();
void resetstats_pthread_mutex_trylock();


struct stats_pthread_mutex_unlock_t {
  int called;           // number of times the system call has been called
  pid_t last_return;    // last return value
  pthread_mutex_t *last_arg; // last mutex passed as argument

};

void init_pthread_mutex_unlock();
void clean_pthread_mutex_unlock();
void resetstats_pthread_mutex_unlock();

struct stats_pthread_mutex_init_t {
  int called;           // number of times the system call has been called
  pid_t last_return;    // last return value
  pthread_mutex_t *last_arg; // last mutex passed as argument

};

void init_pthread_mutex_init();
void clean_pthread_mutex_init();
void resetstats_pthread_mutex_init();

struct stats_pthread_mutex_destroy_t {
  int called;           // number of times the system call has been called
  pid_t last_return;    // last return value
  pthread_mutex_t *last_arg; // last mutex passed as argument

};

void init_pthread_mutex_destroy();
void clean_pthread_mutex_destroy();
void resetstats_pthread_mutex_destroy();

void init_mutex();
void clean_mutex();
void resetstats_mutex();

#endif // __WRAP_MUTEX_H_

