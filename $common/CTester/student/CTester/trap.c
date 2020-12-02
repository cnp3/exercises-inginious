/*
 * Functions for creating and destroying trapped buffers.
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

#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "trap.h"

void *trap_buffer(size_t size, int type, int flags, void *data)
{
    size_t pagesize = sysconf(_SC_PAGESIZE);
    int nb_pages = (size / pagesize) + 2;
    void *ptr = mmap(NULL, pagesize * nb_pages, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }

    void *buf_start;
    if (type == TRAP_LEFT) {
        buf_start = ptr + pagesize;
        mprotect(ptr, pagesize, PROT_NONE);
    } else if (type == TRAP_RIGHT) {
        buf_start = ptr + (nb_pages - 1) * pagesize - size;
        mprotect(ptr + (nb_pages - 1)*pagesize, pagesize, PROT_NONE);
    } else {
        return NULL;
    }

    if (data != NULL)
        memcpy(buf_start, data, size);

    mprotect(buf_start, size, flags);

    return buf_start;
}

int free_trap(void *ptr, size_t size)
{
    return munmap(ptr, size);
}
