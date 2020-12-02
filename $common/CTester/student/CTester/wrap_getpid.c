/*
 * Wrapper for getpid.
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

// pid_t getpid(void);

pid_t __real_getpid(void);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;


void init_getpid() {
  // nothing to do
}

void clean_getpid() {
  // nothing to do
}

void resetstats_getpid() {
  stats.getpid.called=0;
  stats.getpid.last_return=0;
}

pid_t __wrap_getpid(void) {
  if(!wrap_monitoring || !monitored.getpid) {
    return __real_getpid();
  }
  // being monitored

  stats.getpid.called++;
  pid_t ret=__real_getpid();
  stats.getpid.last_return=ret;
  return ret;

}


