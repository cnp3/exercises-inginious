/**
 * @file trap.h
 * Functions for creating and destroying trapped buffers.
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

#ifndef __CTESTER_TRAP_H__
#define __CTESTER_TRAP_H__

/**
 * Defines the type of the trapped buffer, i.e. which end of the buffer
 * is trapped.
 */
enum {
    TRAP_LEFT, ///< Defines that the left (lower address) page of the buffer is trapped
    TRAP_RIGHT ///< Defines that the right (upper address) page of the buffer is trapped
};


/**
 * Creates a trapped buffer, of specified size, of the specified type, with
 * the specified data optionally pre-filled, and with the specified
 * access rights flag (from mprotect) applied to the buffer. The trapped page
 * has PROT_NONE permissions. Returns the start address of the useful
 * part of the buffer (i.e., not the trapped page, if TRAP_LEFT).
 */
void *trap_buffer(size_t size, int type, int flags, void *data);

/**
 * Frees the trapped buffer referenced to by ptr. Note that for TRAP_LEFT,
 * this is not the same pointer as the one returned by trap_buffer; rather,
 * this is the address of the trapped page. Also, the size is not the size
 * of the buffer itself, but of all the pages of the buffer+the trapped page.
 */
int free_trap(void *ptr, size_t size);

#endif // __CTESTER_TRAP_H__

