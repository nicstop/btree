#include <stdio.h>
#include <stdlib.h>

#define XLIB_CORE_IMPLEMENTATION
#include "xlib/core/core.h"

#define XLIB_CORE_ARENA_IMPLEMENTATION
#include "xlib/core/arena.h"

#define BTREE_IMPLEMENTATION
#include "btree.h"

int 
main(int argc, char *argv[])
{
    U32 i;

    BTree btree;

    static U32 ids[] = { 48, 85, 45, 92, 26, 49, 27, 22, 10, 93, 94, 96, 97, 98, 39, 83, 52, 73, 84, 76, 99, 32, 33, 75, 78 }; 

    static U8 buffer[1024 * 4];
    x_arena arena;

    x_arena_init(&arena, &buffer[0], sizeof(buffer));

    printf("B-Tree Stats:\n");
    printf("Key count = %d\n", BTREE_KEY_COUNT);
    printf("Node count = %d\n", BTREE_NODE_COUNT);
    printf("------------------------------\n");

    for (i = 0; i < x_countof(ids); ++i) {
        U32 k;

        printf("Inserting %d...", ids[i]);
        bt_insert(&btree, ids[i], &ids[i], sizeof(ids[i]), &arena);

        for (k = 0; k < i; ++k) {
            if (bt_search(&btree, ids[k]) != NULL) {
            } else {
                printf("error, insert broke node search. Cannot find %d\n", ids[k]);
                break;
            }
        }

        if (k == i) {
            printf("OK.\n");
        }
    }

    return 0;
}

