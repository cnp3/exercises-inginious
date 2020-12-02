/**
 * @file wrap_getpid.h
 * Wrapper for getpid.
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
#ifndef __WRAP_GETPID_H_
#define __WRAP_GETPID_H_

#include <sys/types.h>
#include <unistd.h>

// never remove statistics from this structure, they could be
// used by existing exercices. You might add some additional information
// if it can help to validate some exercices

/**
 * Statistics structure for the getpid system call
 */
struct stats_getpid_t {
  int called;           ///< number of times the system call has been called
  pid_t last_return;    ///< last return value for getpid

};

/**
 * Initializes the structures needed for the wrapper of the getpid system call
 */
void init_getpid();

/**
 * Frees up any structure that was needed for the wrapper
 * of the getpid system call
 */
void clean_getpid();

/**
 * Resets the statistics associated with the getpid system call
 */
void resetstats_getpid();

#endif // __WRAP_GETPID_H_

