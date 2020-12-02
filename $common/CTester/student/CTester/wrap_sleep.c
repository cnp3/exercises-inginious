/*
 * Wrapper for sleep.
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

// unsigned int sleep(unsigned int seconds);

unsigned int __real_sleep(unsigned int);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;


void init_sleep() {
  // nothing to do
}

void clean_sleep() {
  // nothing to do
}

void resetstats_sleep() {
  stats.sleep.called=0;
  stats.sleep.last_return=0;
  stats.sleep.last_arg=0;
}

unsigned int __wrap_sleep(unsigned int time) {
  if(!wrap_monitoring || !monitored.sleep) {
    return __real_sleep(time);
  }

  stats.sleep.called++;
  stats.sleep.last_arg = time;
  // being monitored
  if (FAIL(failures.sleep)) {
    failures.sleep=NEXT(failures.sleep);
    stats.sleep.last_return=failures.sleep_ret;
    return failures.sleep_ret;
  }
  failures.sleep=NEXT(failures.sleep);
  // did not fail

  unsigned int ret=__real_sleep(time);
  stats.sleep.last_return=ret;
  return ret;
}


