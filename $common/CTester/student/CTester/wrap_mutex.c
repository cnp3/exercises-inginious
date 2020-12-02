/*
 * Wrapper for the POSIX thread mutex functions.
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
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "wrap.h" // system call wrapper
#include <pthread.h>


//int pthread_mutex_lock(pthread_mutex_t *mutex);
//int pthread_mutex_trylock(pthread_mutex_t *mutex);
//int pthread_mutex_unlock(pthread_mutex_t *mutex);


int __real_pthread_mutex_lock(pthread_mutex_t *mutex);
int __real_pthread_mutex_trylock(pthread_mutex_t *mutex);
int __real_pthread_mutex_unlock(pthread_mutex_t *mutex);
int __real_pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int __real_pthread_mutex_destroy(pthread_mutex_t *mutex);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;


void init_pthread_mutex_init() {
  // nothing to do
}
void init_pthread_mutex_lock() {
  // nothing to do
}
void init_pthread_mutex_trylock() {
  // nothing to do
}
void init_pthread_mutex_unlock() {
  // nothing to do
}
void init_pthread_mutex_destroy() {
  // nothing to do
}
void init_mutex() {
  init_pthread_mutex_init();
  init_pthread_mutex_lock();
  init_pthread_mutex_trylock();
  init_pthread_mutex_unlock();
  init_pthread_mutex_destroy();
}

void clean_pthread_mutex_init() {
  // nothing to do
}
void clean_pthread_mutex_lock() {
  // nothing to do
}
void clean_pthread_mutex_trylock() {
  // nothing to do
}
void clean_pthread_mutex_unlock() {
  // nothing to do
}
void clean_pthread_mutex_destroy() {
  // nothing to do
}
void clean_mutex() {
  clean_pthread_mutex_init();
  clean_pthread_mutex_lock();
  clean_pthread_mutex_trylock();
  clean_pthread_mutex_unlock();
  clean_pthread_mutex_destroy();
}

void resetstats_pthread_mutex_init() {
  stats.pthread_mutex_init.called=0;
  stats.pthread_mutex_init.last_return=0;
}
void resetstats_pthread_mutex_lock() {
  stats.pthread_mutex_lock.called=0;
  stats.pthread_mutex_lock.last_return=0;
}
void resetstats_pthread_mutex_trylock() {
  stats.pthread_mutex_trylock.called=0;
  stats.pthread_mutex_trylock.last_return=0;
}
void resetstats_pthread_mutex_unlock() {
  stats.pthread_mutex_unlock.called=0;
  stats.pthread_mutex_unlock.last_return=0;
}
void resetstats_pthread_mutex_destroy() {
  stats.pthread_mutex_destroy.called=0;
  stats.pthread_mutex_destroy.last_return=0;
}
void resetstats_mutex() {
  resetstats_pthread_mutex_init();
  resetstats_pthread_mutex_lock();
  resetstats_pthread_mutex_trylock();
  resetstats_pthread_mutex_unlock();
  resetstats_pthread_mutex_destroy();
}

int __wrap_pthread_mutex_destroy(pthread_mutex_t *mutex) {
  if(!wrap_monitoring || !monitored.pthread_mutex_destroy) {
    return __real_pthread_mutex_destroy(mutex);
  }
  // being monitored

  stats.pthread_mutex_destroy.called++;
  int ret=__real_pthread_mutex_destroy(mutex);
  stats.pthread_mutex_destroy.last_arg=mutex;
  stats.pthread_mutex_destroy.last_return=ret;
  return ret;
}


int __wrap_pthread_mutex_init(pthread_mutex_t *restrict mutex,
                       const pthread_mutexattr_t *restrict attr) {
  if(!wrap_monitoring || !monitored.pthread_mutex_init) {
    return __real_pthread_mutex_init(mutex,attr);
  }
  // being monitored

  stats.pthread_mutex_init.called++;
  int ret=__real_pthread_mutex_init(mutex,attr);
  stats.pthread_mutex_init.last_arg=mutex;
  stats.pthread_mutex_init.last_return=ret;
  return ret;

}


pid_t __wrap_pthread_mutex_lock(pthread_mutex_t *mutex) {
  if(!wrap_monitoring || !monitored.pthread_mutex_lock) {
    return __real_pthread_mutex_lock(mutex);
  }
  // being monitored

  stats.pthread_mutex_lock.called++;
  int ret=__real_pthread_mutex_lock(mutex);
  stats.pthread_mutex_lock.last_arg=mutex;
  stats.pthread_mutex_lock.last_return=ret;
  return ret;

}

pid_t __wrap_pthread_mutex_trylock(pthread_mutex_t *mutex) {
  if(!wrap_monitoring || !monitored.pthread_mutex_trylock) {
    return __real_pthread_mutex_trylock(mutex);
  }
  // being monitored

  stats.pthread_mutex_trylock.called++;
  int ret=__real_pthread_mutex_trylock(mutex);
  stats.pthread_mutex_trylock.last_arg=mutex;
  stats.pthread_mutex_trylock.last_return=ret;
  return ret;

}

pid_t __wrap_pthread_mutex_unlock(pthread_mutex_t *mutex) {
  if(!wrap_monitoring || !monitored.pthread_mutex_unlock) {
    return __real_pthread_mutex_unlock(mutex);
  }
  // being monitored

  stats.pthread_mutex_unlock.called++;
  int ret=__real_pthread_mutex_unlock(mutex);
  stats.pthread_mutex_unlock.last_arg=mutex;
  stats.pthread_mutex_unlock.last_return=ret;
  return ret;

}
