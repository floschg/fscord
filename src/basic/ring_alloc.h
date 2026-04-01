#ifndef FSCORD_RING_ALLOCATOR_H
#define FSCORD_RING_ALLOCATOR_H

#include "basic/basic.h"


typedef struct {
    u32 index0;
    u32 cur_count;
    u32 max_count;
    u32 single_size;
    void *data;
} RingAlloc;

void  ring_alloc_init(RingAlloc *ra, size_t single_size, size_t max_count, void *data);
void  ring_alloc_clear(RingAlloc *ra);
void *ring_alloc_push(RingAlloc *ra);


typedef struct {
    u32 index0;
    u32 count_remaining;
    u32 count_allocated;
    u32 single_size;
    void *data;
} RingAllocIt;

void ring_alloc_it_init(RingAllocIt *it, RingAlloc *ra);
void *ring_alloc_it_next(RingAllocIt *it);


#endif // FSCORD_RING_ALLOCATOR_H
