#ifndef __CTESTER_TRAP_H__
#define __CTESTER_TRAP_H__

enum {
    TRAP_LEFT,
    TRAP_RIGHT
};


void *trap_buffer(size_t size, int type, int flags, void *data);
int free_trap(void *ptr, size_t size);

#endif // __CTESTER_TRAP_H__

