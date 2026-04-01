#include "basic/ring_alloc.h"

#include <string.h>


void *
ring_alloc_push(RingAlloc *ra)
{
    u32 index = (ra->index0 + ra->cur_count) % ra->max_count;
    if (ra->cur_count < ra->max_count) {
        ra->cur_count += 1;
    }
    else {
        ra->index0 += 1;
        if (ra->index0 >= ra->max_count) {
            ra->index0 = 0;
        }
    }
    void *result = (u8*)ra->data + ra->single_size * index;
    return result;
}

void
ring_alloc_clear(RingAlloc *ra)
{
    ra->index0 = 0;
    ra->cur_count = 0;
}

void
ring_alloc_init(RingAlloc *ra, size_t single_size, size_t max_count, void *data)
{
    ra->index0 = 0;
    ra->cur_count = 0;
    ra->max_count = max_count;
    ra->single_size = single_size;
    ra->data = data;
}


void *
ring_alloc_it_next(RingAllocIt *it)
{
    if (!it->count_remaining) {
        return 0;
    }

    u32 index = (it->index0 + it->count_remaining - 1) % it->count_allocated;
    void *result = (u8*)it->data + it->single_size * index;

    it->count_remaining--;
    return result;
}

void
ring_alloc_it_init(RingAllocIt *it, RingAlloc *ra)
{
    it->index0 = ra->index0;
    it->count_allocated = ra->cur_count;
    it->count_remaining = ra->cur_count;
    it->single_size = ra->single_size;
    it->data = ra->data;
}

