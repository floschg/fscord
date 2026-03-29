#ifndef ARENA_H
#define ARENA_H


#include <basic/basic.h>

typedef struct {
    size_t size_used;
    size_t size_max;
    u8 *memory;
} Arena;


typedef struct {
    Arena *arena;
    u64 pos;
} ArenaTmp;


void    arena_init(Arena *arena, u64 size);
void    arena_deinit(Arena *arena);

void    arena_align(Arena *arena, u64 alignment);
void*   arena_push(Arena *arena, u64 size);
void    arena_pop_to(Arena *arena, u64 pos);
void    arena_pop_size(Arena *arena, u64 size);

void    arena_zero(Arena *arena);
void    arena_clear(Arena *arena);


ArenaTmp arena_tmp_begin(Arena *arena);
void     arena_tmp_end(ArenaTmp tmp);


#endif // MEM_ARENA_H
