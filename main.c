#define BT_IMPLEMENTATION
#include "btree.h"

#if 0
#include <stdio.h>
#include <stdlib.h>

#if defined(__MACH__)
    #include <stdlib.h>
#else
    #include <malloc.h>
#endif

#define XLIB_CORE_IMPLEMENTATION
#include "xlib/core/core.h"

#define XLIB_CORE_ARENA_IMPLEMENTATION
#include "xlib/core/arena.h"

#if 1
static U32 ids[] = { 48, 85, 45, 92, 26, 49, 27, 22, 10, 93, 94, 96, 97, 98, 39, 83, 52, 73, 84, 76, 99, }; //32, 33, 75, 78, 102, 33, 1, 5, 8, 9, 13, 4, 25 }; 
#else
#include "test_set.h"
#endif

static S32 memory_usage = 0;

BTREE_MALLOC_SIG(test_malloc)
{
#if 0
    static char memory[1024 * 16];
    static U32 cursor = 0;

    void *result;

    x_assert(cursor + size <= sizeof(memory));
    result = (void *)(memory + cursor);
    cursor += size;

    return result;
#else
    void *result;

    result = malloc(size + sizeof(U32));
    *(U32 *)result = size;
    result = (void *)((U8 *)result + sizeof(U32));
    memory_usage += size;
    return result;
#endif
}

BTREE_FREE_SIG(test_free)
{
    if (ptr != NULL) {
        void *ptr_header = (void *)((U8 *)ptr - sizeof(U32)); 
        U32 ptr_size = *(U32 *)(ptr_header);
        memory_usage -= ptr_size;
        x_assert(memory_usage >= 0);
        x_memset(ptr, 0xfe, ptr_size);
        free(ptr_header);
    }
}

BTREE_REALLOC_SIG(test_realloc)
{
    x_assert_msg("no realloc, increase arena size");
}

BTREE_VISIT_KEYS_SIG(test_visit_keys)
{
    printf("Visited %d\n", id);
    return bt_true;
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
        bt_insert(&btree, ids[i], &ids[i]);

#if 1
        for (k = 0; k < i; ++k) {
            if (bt_search(&btree, ids[k]) != NULL) {
            } else {
                printf("error, insert broke node search. Cannot find %d\n", ids[k]);
                return bt_false;
            }
        }

        if (k == i) {
            printf("OK.\n");
        }
#endif
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

    bt_visit_keys(&btree, BTreeVisitNodes_TopDown, NULL, test_visit_keys);

    bt_destroy(&btree);
    x_assert(memory_usage == 0);

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

#if 1
    for (i = 0; i < x_countof(ids); ++i) {
        if (!test_id(ids[i])) {
            printf("Test failed for %d\n", ids[i]);
            break;
        }
    }
    if (i == x_countof(ids)) {
        printf("-------------------------\n");
        printf("All tests passed!\n");
    }
#else
    test_id(85);
#endif

    return 0;
}
#else
int main(int argc, char *argv[])
{
    return 0;
}
#endif
