#include <basic/arena.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

void
arena_init(Arena *arena, u64 size)
{
    arena->size_used = 0;
    arena->size_max = size;
    arena->memory = malloc(size);
}

void
arena_deinit(Arena *arena)
{
    arena->size_used = 0;
    arena->size_max = 0;
    free(arena->memory);
}

void
arena_align(Arena *arena, u64 alignment)
{
    // @Incomplete
}

void*
arena_push(Arena *arena, u64 size)
{
    void *result = arena->memory + arena->size_used;
    arena->size_used += size;
    assert(arena->size_used <= arena->size_max);
    return result;
}

void
arena_pop_to(Arena *arena, u64 pos)
{
    arena->size_used = pos;
}

void
arena_pop_size(Arena *arena, u64 size)
{
    arena->size_used -= size;
}

void
arena_zero(Arena *arena)
{
    memset(arena->memory, 0, arena->size_used);
}

void
arena_clear(Arena *arena)
{
    arena->size_used = 0;
}


ArenaTmp
arena_tmp_begin(Arena *arena)
{
    ArenaTmp tmp;
    tmp.arena = arena;
    tmp.pos = arena->size_used;
    return tmp;
}

void
arena_tmp_end(ArenaTmp tmp)
{
    tmp.arena->size_used = tmp.pos;
}

