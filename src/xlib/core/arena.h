/*
 * $Creator: Nikita Smith
 * $Date: Oct 25 2019
 */

#ifndef XLIB_CORE_ARENA_INCLUDE_H
#define XLIB_CORE_ARENA_INCLUDE_H

typedef struct x_arena {
    U32 len;
    U32 cap;
    void *memory;
} x_arena;

typedef struct x_arena_temp {
    U32 len;
    x_arena *arena;
} x_arena_temp;

x_api void
x_arena_init(x_arena *arena, void *memory, U32 memory_size);

#define x_arena_push(arena, size) x_arena_push_(arena, size, 8)
#define x_arena_push_struct(arena, type) (type *)x_arena_push(arena, sizeof(type))
#define x_arena_push_array(arena, type, count) (type *)x_arena_push(arena, sizeof(type)*count)
x_api void *
x_arena_push_(x_arena *arena, U32 size, U32 align);

x_api x_arena_temp
x_arena_temp_begin(x_arena *arena);

x_api void
x_arena_temp_end(x_arena_temp temp);

#endif /* XLIB_CORE_ARENA_INCLUDE_H */

#if defined(XLIB_CORE_ARENA_IMPLEMENTATION)

x_api void
x_arena_init(x_arena *arena, void *memory, U32 memory_size)
{
    arena->len = 0;
    arena->cap = memory_size;
    arena->memory = memory;
}

#define x_arena_push(arena, size) x_arena_push_(arena, size, 8)
#define x_arena_push_struct(arena, type) (type *)x_arena_push(arena, sizeof(type))
#define x_arena_push_array(arena, type, count) (type *)x_arena_push(arena, sizeof(type)*count)
x_api void *
x_arena_push_(x_arena *arena, U32 size, U32 align)
{
    void *result = 0;

    if (size > 0) {
        UMM ptr = (UMM)arena->memory + arena->len;

        if (align > 0) {
            UMM mask = align - 1;
            align -= ptr & mask;
        }

        if (size + align <= arena->cap) {
            result = (void *)(ptr + align);
            arena->len += size + align;
        }
    }

    return result;
}

x_api x_arena_temp
x_arena_temp_begin(x_arena *arena)
{
    x_arena_temp result;
    result.len = arena->len;
    result.arena = arena;
    return result;
}

x_api void
x_arena_temp_end(x_arena_temp temp)
{
    temp.arena->len = temp.len;
}

#endif /* XLIB_CORE_ARENA_IMPLEMENTATION */
