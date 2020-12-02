/**
 * @file wrap_sleep.h
 * Wrapper for sleep.
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

#ifndef __WRAP_SLEEP_H_
#define __WRAP_SLEEP_H_

#include <sys/types.h>
#include <unistd.h>

// never remove statistics from this structure, they could be
// used by existing exercices. You might add some additional information
// if it can help to validate some exercices

/**
 * Statistics structure for the sleep function.
 */
struct stats_sleep_t {
  int called;                ///< number of times the system call has been called
  unsigned int last_return;  ///< last return value for sleep
  unsigned int last_arg;     ///< last argument value for sleep
};

/**
 * Initializes the wrapper and statistics for the sleep function.
 */
void init_sleep();

/**
 * Cleans the wrapper for the sleep function.
 */
void clean_sleep();

/**
 * Resets the statistics for the sleep function.
 */
void resetstats_sleep();

#endif // __WRAP_SLEEP_H_

