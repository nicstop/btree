#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#define XLIB_CORE_IMPLEMENTATION
#include "xlib/core/core.h"

#define XLIB_CORE_ARENA_IMPLEMENTATION
#include "xlib/core/arena.h"

#define BTREE_IMPLEMENTATION
#include "btree.h"

static U32 ids[] = { 48, 85, 45, 92, 26, 49, 27, 22, 10, 93, 94, 96, 97 }; // , 98, 39, 83, 52, 73, 84, 76, 99, 32, 33, 75, 78 }; 

BTREE_MALLOC_SIG(test_malloc)
{
    return malloc(size);
}

BTREE_FREE_SIG(test_free)
{
    free(ptr);
}

BTREE_REALLOC_SIG(test_realloc)
{
    x_assert_msg("no realloc, increase arena size");
}

bt_bool
test_id(U32 test_id)
{
    U32 i;

    BTreeAllocator allocator;
    BTree btree;

    allocator.alloc_memory_context = NULL;
    allocator.alloc_memory = test_malloc;

    allocator.free_memory_context = NULL;
    allocator.free_memory = test_free;

    allocator.realloc_memory_context = NULL;
    allocator.realloc_memory = test_realloc;

    bt_create(&btree, &allocator);

    for (i = 0; i < x_countof(ids); ++i) {
        U32 k;

        printf("Inserting %d...", ids[i]);
        bt_insert(&btree, ids[i], &ids[i], sizeof(ids[i]));

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

    printf("---------------------------\n");
    printf("Deleting %d\n", test_id);

    bt_delete(&btree, test_id);
    for (i = 0; i < x_countof(ids); ++i) {
        if (bt_search(&btree, ids[i])) {
            if (ids[i] == test_id) {
                printf("Error: Deleted from %d, but search still found it.\n", ids[i]);
                return bt_false;
            } else {
                printf("Found %d\n", ids[i]);
            }
        } else {
            if (ids[i] != test_id) {
                printf("Could not find %d, delete has corrupted tree structure.\n", ids[i]);
                return bt_false;
            }
        }
    }

    bt_destroy(&btree);

    return bt_true;
}

int 
main(int argc, char *argv[])
{
    U32 i;

    printf("B-Tree Stats:\n");
    printf("Key count = %d\n", BTREE_KEY_COUNT);
    printf("Node count = %d\n", BTREE_NODE_COUNT);
    printf("------------------------------\n");

#if 0
    for (i = 0; i < x_countof(ids); ++i) {
        if (!test_id(ids[i])) {
            printf("Test failed for %d\n", ids[i]);
            break;
        }
    }
#else
    test_id(93);
#endif

    return 0;
}

