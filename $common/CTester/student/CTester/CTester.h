/**
 * @file wrap.h
 * Main CTester file, containing all base functions and structures.
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

#ifndef __CTESTER_MAIN_H__
#define __CTESTER_MAIN_H__

#include <sys/mman.h>
#include <CUnit/CUnit.h>
#include <setjmp.h>

#include "wrap.h"
#include "trap.h"

#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)

#define RUN(...) void *ptr_tests[] = {__VA_ARGS__}; return run_tests(argc, argv, ptr_tests, sizeof(ptr_tests)/sizeof(void*))
#define BAN_FUNCS(...) 
#define SANDBOX_BEGIN sandbox_begin(); if(sigsetjmp(segv_jmp,1) == 0) { (void)0
#define SANDBOX_END } else { \
                             sandbox_fail(); \
                           } \
                           sandbox_end()


// Hidden by macros
int run_tests(int argc, char *argv[], void *tests[], int nb_tests);
int sandbox_begin();
void sandbox_fail();
void sandbox_end();

// To use inside tests
void set_test_metadata(char *problem, char *descr, unsigned int weight);
void push_info_msg(char *msg);
void set_tag(char *tag);


// Set to true to enable monitoring features
bool wrap_monitoring = false;

struct wrap_stats_t stats;
struct wrap_monitor_t monitored;
struct wrap_fail_t failures;
struct wrap_log_t logs;

void reinit_all_stats()
{
	memset(&stats, 0, sizeof(stats));
}

void reinit_all_monitored()
{
	memset(&monitored, 0, sizeof(monitored));
}

/**
 * Readable file descriptors that contain the student's code outputs.
 */
int stdout_cpy, stderr_cpy;

FILE *fstdout, *fstderr;

sigjmp_buf segv_jmp;

#endif // __CTESTER_MAIN_H__

