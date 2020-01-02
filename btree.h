#ifndef BTREE_HEADER_INCLUDE
#define BTREE_HEADER_INCLUDE

/* 
 * B-Tree single-header lib by Nikita Smith
 *
 * -------------------------------------------------------------------------------- 
 *
 */

/* NOTE(nick): Here you can pick which order you want for this tree. */
#define BTREE_NODE_COUNT 4 
#define BTREE_KEY_COUNT ((BTREE_NODE_COUNT) - 1) 

#define BTREE_API
#define BTREE_INTERNAL static

#include <stdint.h>
typedef uint8_t bt_u08;
typedef int8_t bt_s08;
typedef uint16_t bt_u16;
typedef int16_t bt_s16;
typedef uint32_t bt_u32;
typedef int32_t bt_s32;
typedef bt_s32 bt_bool;

#define bt_false 0
#define bt_true  1

#define BTREE_INVALID_ID 0xFFFFFFFF
typedef int BTreeKeyID;

#ifndef BTREE_CUSTOM_MEM
    #include <string.h>
    #define bt_memcopy(dst, src, size)  memcpy(dst, src, size)
    #define bt_memmove(dst, src, size)  memcpy(dst, src, size); memset(src, 0, size)
    #define bt_memset(ptr, value, size) memset(ptr, value, size)
#endif

#define BTREE_ASSERT(cnd) if (!(cnd)) { *((volatile int *)0); }
#define BTREE_ASSERT_FAILURE(message) BTREE_ASSERT(0)

#define BTREE_MALLOC_SIG(name) void * name(void *user_context, size_t size)
typedef BTREE_MALLOC_SIG(bt_malloc_sig);

#define BTREE_FREE_SIG(name) void name(void *user_context, void *ptr)
typedef BTREE_FREE_SIG(bt_free_sig);

#define BTREE_REALLOC_SIG(name) void * name(void *user_context, void *ptr, size_t size)
typedef BTREE_REALLOC_SIG(bt_realloc_sig);

#define BTREE_VISIT_KEYS_SIG(name) bt_bool name(void *user_context, BTreeKeyID id, const void *data)
typedef BTREE_VISIT_KEYS_SIG(bt_visit_keys_sig);

typedef enum {
    BTreeErrorCode_Ok,
    BTreeErrorCode_AllocationFailed,
    BTreeErrorCode_OpDenied,
    BTreeErrorCode_RebalanceFailed,
    BTreeErrorCode_IDNotFound
} BTreeErrorCode;

typedef enum {
    BTreeVisitNodes_Null,
    BTreeVisitNodes_TopDown,
    BTreeVisitNodes_BottomUp
} BTreeVisitNodesMode;

typedef struct BTreeKey {
    BTreeKeyID id;
    void const *data;
} BTreeKey;

typedef struct BTreeNode {
    bt_u08 key_count;
    BTreeKey keys[BTREE_KEY_COUNT];
    struct BTreeNode *subs[BTREE_NODE_COUNT];
} BTreeNode;

typedef struct BTreeStackFrame {
    bt_u08 key_index;
    BTreeNode *node;
    struct BTreeStackFrame *next;
} BTreeStackFrame;

typedef struct BTreeAllocator {
    void *alloc_memory_context;
    bt_malloc_sig *alloc_memory;

    void *free_memory_context;
    bt_free_sig *free_memory;

    void *realloc_memory_context;
    bt_realloc_sig *realloc_memory;
} BTreeAllocator;

typedef struct BTree {
    BTreeAllocator allocator;

    bt_u32 frames_count;
    bt_u32 frames_max;
    BTreeStackFrame *frames;

    BTreeNode *root;
} BTree;

BTREE_API BTreeErrorCode
bt_create(BTree *tree, BTreeAllocator *allocator);

BTREE_API BTreeErrorCode
bt_destroy(BTree *tree);

BTREE_API BTreeKey *
bt_search(BTree *tree, bt_u32 id);

BTREE_API BTreeErrorCode
bt_insert(BTree *tree, bt_u32 id, const void *data);

BTREE_API BTreeErrorCode
bt_delete(BTree *tree, bt_u32 id);

BTREE_API BTreeErrorCode
bt_visit_keys(BTree *tree, BTreeVisitNodesMode mode, void *user_context, bt_visit_keys_sig *visit);

/* -------------------------------------------------------------------------------- */

BTREE_INTERNAL void
bt_debug_printf(const char *format, ...);

BTREE_INTERNAL void *
bt_malloc(BTree *tree, size_t size);

BTREE_INTERNAL void
bt_free(BTree *tree, void *ptr);

BTREE_INTERNAL void *
bt_realloc(BTree *tree, void *ptr, size_t size);

BTREE_INTERNAL BTreeNode *
bt_new_node(BTree *tree);

BTREE_INTERNAL bt_bool
bt_is_node_leaf(BTreeNode *node);

BTREE_INTERNAL void
bt_shift_keys_right(BTreeNode *node, bt_u32 start_key);

BTREE_INTERNAL BTreeErrorCode
bt_push_stack_frame(BTree *tree, BTreeNode *node, BTreeKeyID key_id);

BTREE_INTERNAL BTreeErrorCode
bt_pop_stack_frame(BTree *tree, BTreeStackFrame *frame_out);

BTREE_INTERNAL BTreeErrorCode
bt_peek_stack_frame(BTree *tree, BTreeStackFrame *frame_out);

BTREE_INTERNAL void
bt_reset_stack(BTree *tree);

BTREE_INTERNAL void
bt_dump_stack(BTree *tree);

BTREE_INTERNAL void
bt_dump_node_keys(BTreeNode *node);

#endif /* BTREE_HEADER_INCLUDE */

#ifdef BTREE_IMPLEMENTATION

BTREE_INTERNAL void
bt_debug_printf(const char *format, ...)
{
#if 0
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}

BTREE_INTERNAL void *
bt_malloc(BTree *tree, size_t size)
{
    return tree->allocator.alloc_memory(tree->allocator.alloc_memory_context, size);
}

BTREE_INTERNAL void
bt_free(BTree *tree, void *ptr)
{
    tree->allocator.free_memory(tree->allocator.free_memory_context, ptr);
}

BTREE_INTERNAL void *
bt_realloc(BTree *tree, void *ptr, size_t size)
{
    return tree->allocator.realloc_memory(tree->allocator.realloc_memory_context, ptr, size);
}

BTREE_INTERNAL BTreeNode *
bt_new_node(BTree *tree)
{
    BTreeNode *node = (BTreeNode *)bt_malloc(tree, sizeof(BTreeNode));
    node->key_count = 0;
    bt_memset(&node->keys[0], 0, sizeof(node->keys));
    bt_memset(&node->subs[0], 0, sizeof(node->subs));
    return node;
}

BTREE_INTERNAL void
bt_node_set_key(BTreeNode *node, bt_u32 key_index, BTreeKeyID id, const void *data)
{
    BTREE_ASSERT(key_index < x_countof(node->keys));
    node->keys[key_index].id = id;
    node->keys[key_index].data = data;
}

BTREE_INTERNAL BTreeKey *
bt_node_get_key(BTreeNode *node, bt_u32 key_index)
{
    BTREE_ASSERT(key_index < x_countof(node->keys));
    return &node->keys[key_index];
}

BTREE_INTERNAL void
bt_node_invalidate_key(BTreeNode *node, bt_u32 key_index)
{
    BTREE_ASSERT(key_index < x_countof(node->keys));
    bt_node_set_key(node, key_index, BTREE_INVALID_ID, NULL);
}

BTREE_INTERNAL void
bt_node_add_key(BTreeNode *node, BTreeKeyID id, const void *data)
{
    if (node->key_count < x_countof(node->keys)) {
        bt_node_set_key(node, node->key_count, id, data);
        node->key_count += 1;
    } else {
        BTREE_ASSERT_FAILURE("cannot add key to a full node");
    }
}

BTREE_INTERNAL void
bt_node_remove_key(BTreeNode *node)
{
    if (node->key_count > 0) {
        bt_node_invalidate_key(node, node->key_count - 1);
        node->key_count -= 1;
    } else {
        BTREE_ASSERT_FAILURE("no keys to remove from an empty node");
    }
}

BTREE_INTERNAL bt_bool
bt_node_set_sub(BTreeNode *node, bt_u32 sub_index, BTreeNode *sub)
{
    bt_bool result = bt_false;

    if (sub_index < x_countof(node->subs)) {
        node->subs[sub_index] = sub;
        result = bt_true;
    } else {
        BTREE_ASSERT_FAILURE("sub index out of bounds");
    }

    return result;
}

BTREE_INTERNAL BTreeNode *
bt_node_get_sub(BTreeNode *node, bt_u32 sub_index)
{
    BTreeNode *result;

    if (sub_index < x_countof(node->subs)) {
        result = node->subs[sub_index];
    } else {
        BTREE_ASSERT("sub index out of bounds, returning NULL");
        result = NULL;
    }

    return result;
}

BTREE_INTERNAL bt_bool
bt_is_node_leaf(BTreeNode *node)
{
    bt_u32 i;
    for (i = 0; i < x_countof(node->subs); ++i) {
        if (node->subs[i] != 0) {
            return bt_false;
        }
    }
    return bt_true;
}

BTREE_INTERNAL void
bt_shift_keys_left(BTreeNode *node, bt_u32 key_index)
{
    bt_u32 i;
    BTREE_ASSERT(node->key_count > 0);
    for (i = key_index; i < node->key_count - 1; ++i) {
        BTreeKey *key;

        key = bt_node_get_key(node, i + 1);
        bt_node_set_key(node, i, key->id, key->data);
        bt_node_invalidate_key(node, i + 1);
    }
}

BTREE_INTERNAL void
bt_shift_keys_right(BTreeNode *node, bt_u32 start_key)
{
    S32 i;
    for (i = x_countof(node->keys) - 2; i >= 0; --i) {
        BTreeKey *key;

        if (i < (S32)start_key) {
            break;
        }
        key = bt_node_get_key(node, i);
        bt_node_set_key(node, i + 1, key->id, key->data);
        bt_node_invalidate_key(node, i);
    }
}

BTREE_INTERNAL void
bt_shift_subs_left(BTreeNode *node, bt_u32 key_index)
{
    S32 i;
    for (i = (S32)key_index; i < (S32)node->key_count; ++i) {
        BTreeNode *sub;
        sub = bt_node_get_sub(node, i + 1);
        bt_node_set_sub(node, i, sub);
        bt_node_set_sub(node, i + 1, NULL);
    }
    bt_node_set_sub(node, node->key_count, NULL);
}

BTREE_INTERNAL void
bt_shift_subs_right(BTreeNode *node, bt_u32 key_index)
{
    S32 i;
    for (i = (S32)node->key_count - 1; i >= (S32)key_index; --i) {
        BTreeNode *sub;

        sub = bt_node_get_sub(node, i);
        bt_node_set_sub(node, i + 1, sub);
        bt_node_set_sub(node, i, NULL);
    }
}

BTREE_INTERNAL BTreeErrorCode
bt_push_stack_frame(BTree *tree, BTreeNode *node, BTreeKeyID key_id)
{
    BTreeStackFrame *frame;

    if (tree->frames_max == 0) {
        tree->frames_count = 0;
        tree->frames_max = 32;
        tree->frames = (BTreeStackFrame *)bt_malloc(tree, sizeof(BTreeStackFrame) * tree->frames_max);
        if (tree->frames == NULL) {
            return BTreeErrorCode_AllocationFailed;
        }
    }

    if (tree->frames_count >= tree->frames_max) {
        tree->frames_max *= 2;
        tree->frames = (BTreeStackFrame *)bt_realloc(tree, tree->frames, sizeof(BTreeStackFrame) * tree->frames_max);
        if (tree->frames == NULL) {
            return BTreeErrorCode_AllocationFailed;
        }
    }

    tree->frames[tree->frames_count].node = node;
    tree->frames[tree->frames_count].key_index = key_id;
    tree->frames_count += 1;

    return BTreeErrorCode_Ok;
}

BTREE_INTERNAL BTreeErrorCode
bt_pop_stack_frame(BTree *tree, BTreeStackFrame *frame_out)
{
    BTreeErrorCode error_code = BTreeErrorCode_OpDenied;

    if (tree->frames_count > 0) {
        *frame_out = tree->frames[--tree->frames_count];
        error_code = BTreeErrorCode_Ok;
    }

    return error_code;
}

BTREE_INTERNAL BTreeErrorCode
bt_peek_stack_frame(BTree *tree, BTreeStackFrame *frame_out)
{
    BTreeErrorCode error_code = BTreeErrorCode_OpDenied;

    if (tree->frames_count > 0) {
        *frame_out = tree->frames[tree->frames_count - 1];
        error_code = BTreeErrorCode_Ok;
    }

    return error_code;
}

BTREE_INTERNAL void
bt_reset_stack(BTree *tree)
{
    tree->frames_count = 0;
}

BTREE_API BTreeErrorCode
bt_create(BTree *tree, BTreeAllocator *allocator)
{
    tree->root = NULL;

    tree->frames = NULL;
    tree->frames_count = 0;
    tree->frames_max = 0;

    tree->allocator = *allocator;

    return BTreeErrorCode_Ok;
}

BTREE_API BTreeErrorCode
bt_destroy(BTree *tree)
{
    BTreeNode *node = tree->root;

    while (node != NULL) {
        if (bt_is_node_leaf(node)) {
            BTreeStackFrame frame;

            bt_free(tree, node);
            node = NULL;
            while (bt_pop_stack_frame(tree, &frame) == BTreeErrorCode_Ok) {
                frame.key_index += 1;

                if (frame.key_index > frame.node->key_count) {
                    /* NOTE(nick): Traversed all sub nodes and returned back to the parent node. */
                    bt_free(tree, frame.node);
                } else {
                    BTreeNode *sub = bt_node_get_sub(frame.node, frame.key_index);
                    if (sub != NULL) {
                        BTreeErrorCode error_code;
                       
                        error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
                        if (error_code != BTreeErrorCode_Ok) {
                            return error_code;
                        }

                        node = sub;
                        BTREE_ASSERT(node->key_count > 0);

                        break;
                    }
                }
            }
        } else {
            BTreeErrorCode error_code;

            /* NOTE(nick): Descending down to a first sub-node */
            BTREE_ASSERT(node->key_count > 0);

            error_code = bt_push_stack_frame(tree, node, 0);
            if (error_code != BTreeErrorCode_Ok) {
                return error_code;
            }

            node = bt_node_get_sub(node, 0);
        }
    }

    bt_free(tree, tree->frames);
    tree->frames_count = 0;
    tree->frames_max = 0;
    tree->frames = NULL;

    tree->root = NULL;

    return BTreeErrorCode_Ok;
}

BTREE_API BTreeKey *
bt_search(BTree *tree, bt_u32 id)
{
    BTreeNode *node = tree->root;

    bt_u32 cmp_counter = 0;

    while (node) {
#if 1
        bt_u32 key_index;

        for (key_index = 0; key_index < node->key_count; ++key_index) {
            if (node->keys[key_index].id == id) {
                return &node->keys[key_index];
            }

            if (id < node->keys[key_index].id) {
                break;
            }
        }

        node = node->subs[key_index];
#else
        bt_u32 min, max, mid;

        min = 0;
        max = node->key_count - 1;
        mid = 0;

        while (min < max) {
            mid = (max - min) / 2;

            if (id < node->keys[mid].id) {
                min = mid + 1;
            } else if (id > node->keys[mid].id) {
                max = mid - 1;
            } else {
                return &node->keys[mid];
            }
        }

        node = node->subs[max];
#endif
    }

    bt_debug_printf("Searching took %d compare(s)\n", cmp_counter);

    return NULL;
}

BTREE_API BTreeErrorCode
bt_insert(BTree *tree, bt_u32 id, const void *data)
{
    BTreeErrorCode error_code = BTreeErrorCode_OpDenied;
    BTreeStackFrame frame;

    if (tree->root == NULL) {
        tree->root = bt_new_node(tree);
        if (tree->root == NULL) {
            return BTreeErrorCode_AllocationFailed;
        }
    }

    {
        BTreeNode *node = tree->root;
        bt_reset_stack(tree);
        while (node) {
            bt_u32 key_index;
            BTreeErrorCode error_code;

            for (key_index = 0; key_index < node->key_count; ++key_index) {
                BTreeKey *key = bt_node_get_key(node, key_index);
                if (key->id == id) {
                    return BTreeErrorCode_Ok;
                } else if (id < key->id) {
                    break;
                }
            }
            /* NOTE(nick): Pushing frame in case we need to split. */
            error_code = bt_push_stack_frame(tree, node, key_index);
            if (error_code != BTreeErrorCode_Ok) {
                return error_code;
            }

            node = bt_node_get_sub(node, key_index);
        }
    }

    error_code = bt_peek_stack_frame(tree, &frame);
    if (error_code != BTreeErrorCode_Ok) {
        return error_code;
    }
    BTREE_ASSERT(frame.node->key_count < x_countof(frame.node->keys));
    bt_shift_keys_right(frame.node, frame.key_index);
    frame.node->key_count += 1;
    bt_node_set_key(frame.node, frame.key_index, id, data);
    error_code = BTreeErrorCode_Ok;

    {
        while (bt_pop_stack_frame(tree, &frame) == BTreeErrorCode_Ok) {
            BTreeNode *node_split = NULL;
            BTreeKey median_key;
            bt_u32 i;

            error_code = BTreeErrorCode_RebalanceFailed;
            if (frame.node->key_count < x_countof(frame.node->keys)) {
                break;
            }

            node_split = bt_new_node(tree);
            if (node_split == NULL) {
                error_code = BTreeErrorCode_AllocationFailed;
                break;
            }

            /* NOTE(nick): Copy upper-half of the sub-nodes to the split node. */
            for (i = x_countof(frame.node->subs) / 2; i < x_countof(frame.node->subs); ++i) {
                BTreeNode *sub = bt_node_get_sub(frame.node, i);
                bt_node_set_sub(node_split, i - x_countof(node_split->subs) / 2, sub);
                bt_node_set_sub(frame.node, i, NULL);
            }

            /* NOTE(nick): Copy upper-half of the keys to the split node. */
            for (i = x_countof(frame.node->keys) / 2; i < x_countof(frame.node->keys); ++i) {
                BTreeKey *key = bt_node_get_key(frame.node, i);
                bt_node_add_key(node_split, key->id, key->data);
                bt_node_invalidate_key(frame.node, i);
                frame.node->key_count -= 1;
            }

            if (node_split->key_count > frame.node->key_count) {
                median_key = *bt_node_get_key(node_split, 0);
                bt_shift_keys_left(node_split, 0);
                node_split->key_count -= 1;
            } else {
                median_key = *bt_node_get_key(frame.node, frame.node->key_count - 1);
                bt_node_invalidate_key(frame.node, frame.node->key_count - 1);
                frame.node->key_count -= 1;
            }

            BTREE_ASSERT(node_split->key_count > 0);
            BTREE_ASSERT(frame.node->key_count > 0);

            { 
                /* NOTE(nick): Inserting a new median key. */

                BTreeStackFrame frame_parent;

                if (bt_peek_stack_frame(tree, &frame_parent) == BTreeErrorCode_Ok) {
                    BTREE_ASSERT(frame_parent.node->key_count < x_countof(frame_parent.node->keys));
                    bt_shift_keys_right(frame_parent.node, frame_parent.key_index);
                    bt_node_set_key(frame_parent.node, frame_parent.key_index, median_key.id, median_key.data);

                    frame_parent.node->key_count += 1;
                    frame_parent.key_index += 1;

                    bt_shift_subs_right(frame_parent.node, frame_parent.key_index);
                    BTREE_ASSERT(bt_node_get_sub(frame_parent.node, frame_parent.key_index) == NULL);
                    bt_node_set_sub(frame_parent.node, frame_parent.key_index, node_split);
                } else {
                    /* NOTE(nick): Splitting reached root node, inserting a new root. */
                    BTreeNode *new_root = bt_new_node(tree);
                    if (new_root != NULL) {
                        bt_node_add_key(new_root, median_key.id, median_key.data);
                        bt_node_set_sub(new_root, 0, frame.node);
                        bt_node_set_sub(new_root, 1, node_split);
                        tree->root = new_root;
                    } else {
                        error_code = BTreeErrorCode_AllocationFailed;
                        break;
                    }
                }
            }
        }
    }

    return error_code;
}

BTREE_API BTreeErrorCode
bt_delete(BTree *tree, bt_u32 id)
{
    BTreeNode *node = tree->root;
    bt_u32 key_index_delete = BTREE_INVALID_ID;
    BTreeNode *node_delete = NULL;

    bt_reset_stack(tree);
    while (node && node_delete == NULL) {
        bt_u32 key_index;
        BTreeErrorCode error_code;

        for (key_index = 0; key_index < node->key_count; ++key_index) {
            BTreeKey *key = bt_node_get_key(node, key_index);
            if (key->id == id) {
                node_delete = node;
                key_index_delete = key_index;
                break;
            } else if (id < key->id) {
                break;
            }
        }

        error_code = bt_push_stack_frame(tree, node, key_index);
        if (error_code != BTreeErrorCode_Ok) {
            return error_code;
        }

        node = bt_node_get_sub(node, key_index);
    }

    if (node_delete == NULL) {
        return BTreeErrorCode_IDNotFound;
    }

    {
        /* NOTE(nick): Picking largest ID on the left branch of node_delete and inserting it
         * instead of the key that is being deleted, because after delete branch becomes
         * unbalanced and to compensate for it we insert largest ID. */

        BTreeNode *node_new_separator = NULL;

        while (node != NULL) {
            BTreeKey *key;
            BTreeKey *key_separator;
            BTreeErrorCode error_code;

            error_code = bt_push_stack_frame(tree, node, node->key_count);
            if (error_code != BTreeErrorCode_Ok) {
                return error_code;
            }

            if (node_new_separator == NULL) {
                node_new_separator = node;
            }

            key = bt_node_get_key(node, node->key_count - 1);
            key_separator = bt_node_get_key(node_new_separator, node_new_separator->key_count - 1);

            if (key->id > key_separator->id) {
                node_new_separator = node;
            }

            node = bt_node_get_sub(node, node->key_count);
        }

        if (node_new_separator != NULL) {
            BTreeKey *key;
            BTreeNode *sub;

            BTREE_ASSERT(node_new_separator->key_count > 0);
            key = bt_node_get_key(node_new_separator, node_new_separator->key_count - 1);
            bt_node_set_key(node_delete, key_index_delete, key->id, key->data);
            bt_node_invalidate_key(node_new_separator, node_new_separator->key_count - 1);
            node_new_separator->key_count -= 1;
        } else {
            bt_shift_keys_left(node_delete, key_index_delete);
            node_delete->key_count -= 1;
        }
    }

    /* NOTE(nick): re-balance tree */
    while (bt_true) {
        BTreeStackFrame frame, frame_parent;
        BTreeNode *node_right, *node_left;
        BTreeNode *node_separator;
        bt_u32 key_index_separator;
        BTreeNode *node_deficient;
        BTreeKey *key;
        BTreeNode *sub;
        
        if (bt_pop_stack_frame(tree, &frame) != BTreeErrorCode_Ok) {
            break;
        }
        if (bt_peek_stack_frame(tree, &frame_parent) != BTreeErrorCode_Ok) {
            break;
        }
        if (frame.node->key_count >  0) {
            break;
        }

        node_deficient = frame.node;
        node_separator = frame_parent.node;
        key_index_separator = frame_parent.key_index;

        node_left = NULL;
        if (key_index_separator > 0) {
            node_left = node_separator->subs[key_index_separator - 1];
        }

        node_right = NULL;
        if ((key_index_separator + 1) < x_countof(node_separator->subs)) {
            node_right = node_separator->subs[key_index_separator + 1];
        }

        if (node_right != NULL && node_right->key_count > 1) {
            BTREE_ASSERT(node_deficient->subs[2] == NULL);
            BTREE_ASSERT(frame_parent.key_index == key_index_separator);

            key = bt_node_get_key(node_separator, key_index_separator);
            bt_node_add_key(node_deficient, key->id, key->data);

            key = bt_node_get_key(node_right, 0);
            bt_node_set_key(node_separator, key_index_separator, key->id, key->data);

            /* NOTE(nick): Don't forget to move sub-node too. It belongs to the key that we 
             * moved from right-node to the deficient-node. */
            sub = bt_node_get_sub(node_right, 0);
            bt_node_set_sub(node_deficient, 1, sub);
            bt_shift_subs_left(node_right, 0);

            bt_shift_keys_left(node_right, 0);
            node_right->key_count -= 1;
        } else if (node_left != NULL && node_left->key_count > 1) {
            BTREE_ASSERT(node_deficient->subs[2] == NULL);
            BTREE_ASSERT(frame_parent.key_index == key_index_separator);
            BTREE_ASSERT(key_index_separator > 0);

            key = bt_node_get_key(node_separator, key_index_separator - 1);
            bt_node_add_key(node_deficient, key->id, key->data);

            bt_shift_subs_right(node_deficient, 0);
            sub = bt_node_get_sub(node_left, node_left->key_count);
            bt_node_set_sub(node_deficient, 0, sub);
            bt_node_set_sub(node_left, node_left->key_count, NULL);

            key = bt_node_get_key(node_left, node_left->key_count - 1);
            bt_node_set_key(node_separator, key_index_separator - 1, key->id, key->data);
            bt_node_invalidate_key(node_left, node_left->key_count - 1);
            node_left->key_count -= 1;
        } else {
            BTreeNode *node_dst;
            BTreeNode *node_src;
            S32 copy_count, i;

            if (node_left != NULL) {
                BTREE_ASSERT(node_left->key_count < x_countof(node_left->keys));
                BTREE_ASSERT(key_index_separator > 0 );

                node_dst = node_left;
                node_src = (node_deficient != NULL) ? node_deficient : node_right;

                /* NOTE(nick): Copy down key from parent to this deficient node. */
                key = bt_node_get_key(node_separator, key_index_separator - 1);
                bt_node_add_key(node_dst, key->id, key->data);
                bt_shift_keys_left(node_separator, key_index_separator - 1);
                bt_shift_subs_left(node_separator, key_index_separator);
            } else {
                node_dst = node_deficient;
                node_src = node_right;
                BTREE_ASSERT(node_dst->key_count == 0);

                node_dst->keys[node_dst->key_count++] = node_separator->keys[key_index_separator];
                BTREE_ASSERT(key_index_separator == frame_parent.key_index);
                bt_shift_keys_left(node_separator, key_index_separator);
            }

            copy_count = (S32)node_src->key_count;
            if ((node_dst->key_count + copy_count) >= x_countof(node_dst->keys)) {
                copy_count = x_countof(node_dst->keys) - node_src->key_count - node_dst->key_count - 1;
            }

            for (i = 0; i < copy_count; ++i) {
                BTREE_ASSERT(node_dst->key_count < x_countof(node_dst->keys));
                BTREE_ASSERT(node_src->key_count < x_countof(node_src->keys));

                key = bt_node_get_key(node_src, i);
                sub = bt_node_get_sub(node_src, i);

                bt_node_set_key(node_dst, node_dst->key_count + i, key->id, key->data);
                bt_node_set_sub(node_dst, node_dst->key_count + i, sub);

                bt_node_invalidate_key(node_src, i);
                bt_shift_subs_left(node_src, i);

                node_src->key_count -= 1;
                node_dst->key_count += 1;
            }

            sub = bt_node_get_sub(node_src, node_src->key_count);
            bt_node_set_sub(node_src, node_src->key_count, NULL);
            bt_node_set_sub(node_dst, node_dst->key_count, sub);

            if (node_src->key_count == 0) {
                if (node_src == tree->root) {
                    bt_free(tree, tree->root);
                    tree->root = node_dst;
                }

                if (node_src == node_right) {
                    BTREE_ASSERT(bt_node_get_sub(node_separator, key_index_separator + 1) == node_src);
                    bt_shift_subs_left(node_separator, key_index_separator + 1);
                }

                bt_free(tree, node_src);
                node_src = NULL;
            }

            BTREE_ASSERT(node_separator->key_count > 0);
            bt_node_invalidate_key(node_separator, node_separator->key_count - 1);
            node_separator->key_count -= 1;
        }
    }

    return BTreeErrorCode_Ok;
}

BTREE_API BTreeErrorCode
bt_visit_keys(BTree *tree, BTreeVisitNodesMode mode, void *user_context, bt_visit_keys_sig *visit)
{
    BTreeNode *node;
    bt_u32 i;

    if (tree->root == NULL) {
        return BTreeErrorCode_Ok;
    } 

    if (visit == NULL) {
        return BTreeErrorCode_OpDenied;
    }

    node = tree->root;
    bt_reset_stack(tree);

    switch (mode) {
    case BTreeVisitNodes_TopDown: {
        while (node != NULL) {
            for (i = 0; i < node->key_count; ++i) {
                BTreeKey *key = bt_node_get_key(node, i);
                if (visit(user_context, key->id, key->data) == bt_false) {
                    return BTreeErrorCode_Ok;
                }
            }

            if (bt_is_node_leaf(node)) {
                BTreeStackFrame frame;

                node = NULL;
                while (bt_pop_stack_frame(tree, &frame) == BTreeErrorCode_Ok) {
                    frame.key_index += 1;
                    if (frame.key_index <= frame.node->key_count) {
                        BTreeNode *sub = bt_node_get_sub(frame.node, frame.key_index);
                        if (sub != NULL) {
                            BTreeErrorCode error_code;

                            error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
                            if (error_code != BTreeErrorCode_Ok) {
                                return error_code;
                            }

                            node = sub;
                            BTREE_ASSERT(node->key_count > 0);
                            break;
                        }
                    }
                }
            } else {
                BTreeErrorCode error_code;

                BTREE_ASSERT(node->key_count > 0);
                error_code = bt_push_stack_frame(tree, node, 0);
                if (error_code != BTreeErrorCode_Ok) {
                    return error_code;
                }

                node = bt_node_get_sub(node, 0);
            }
        }
    } break;

    case BTreeVisitNodes_BottomUp: {
        while (node != NULL) {
            if (bt_is_node_leaf(node)) {
                BTreeStackFrame frame;

                for (i = 0; i < node->key_count; ++i) {
                    BTreeKey *key = bt_node_get_key(node, i);
                    if (visit(user_context, key->id, key->data) == bt_false) {
                        return BTreeErrorCode_Ok;
                    }
                }

                node = NULL;
                while (bt_pop_stack_frame(tree, &frame) == BTreeErrorCode_Ok) {
                    frame.key_index += 1;

                    if (frame.key_index > frame.node->key_count) {
                        for (i = 0; i < frame.node->key_count; ++i) {
                            BTreeKey *key = bt_node_get_key(frame.node, i);
                            if (visit(user_context, key->id, key->data) == bt_false) {
                                return BTreeErrorCode_Ok;
                            }
                        }
                    } else {
                        BTreeNode *sub = bt_node_get_sub(frame.node, frame.key_index);
                        if (sub != NULL) {
                            BTreeErrorCode error_code;

                            error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
                            if (error_code != BTreeErrorCode_Ok) {
                                return error_code;
                            }

                            node = sub;
                            BTREE_ASSERT(node->key_count > 0);
                            break;
                        }
                    }
                }
            } else {
                BTreeErrorCode error_code;

                BTREE_ASSERT(node->key_count > 0);
                error_code = bt_push_stack_frame(tree, node, 0);
                if (error_code != BTreeErrorCode_Ok) {
                    return error_code;
                }

                node = bt_node_get_sub(node, 0);
            }
        }
    } break;

    default: break;
    }

    return BTreeErrorCode_Ok;
}

BTREE_INTERNAL void
bt_dump_stack(BTree *tree)
{
    bt_u32 i;

    bt_debug_printf("Dumping node stack:\n");
    for (i = 0; i < tree->frames_count; ++i) {
        BTreeStackFrame *frame = &tree->frames[i];
        if (frame->key_index < x_countof(frame->node->keys)) {
            bt_debug_printf("%d: key_index: %d, id: %d\n", i, frame->key_index, frame->node->keys[frame->key_index].id);
        } else {
            bt_debug_printf("%d: key_index: %d, no id\n", i, frame->key_index);
        }
    }
}

BTREE_INTERNAL void
bt_dump_node_keys(BTreeNode *node)
{
    bt_u32 i;
    for (i = 0; i < node->key_count; ++i) {
        bt_debug_printf("%d ", node->keys[i].id);
    }
    bt_debug_printf("\n");

    for (i = 0; i < x_countof(node->subs); ++i) {
        if (node->subs[i] != NULL) {
            bt_u32 k;
            bt_debug_printf("Sub key %d, key_count = %d, 0x%llx:\n", i, node->subs[i]->key_count, node->subs[i]);
            for (k = 0; k < node->subs[i]->key_count; ++k) {
                bt_debug_printf("%d ", node->subs[i]->keys[k].id);
            }
            bt_debug_printf("\n");
        }
    }
}

BTREE_INTERNAL void
bt_dump_tree(BTree *tree, x_arena *arena)
{
    BTreeNode *node = tree->root;
    bt_u32 depth = 0;
    x_arena_temp temp;
    BTreeStackFrame *stack = 0;

    temp = x_arena_temp_begin(arena);

    while (node) {
        bt_u32 i;

        for (i = 0; i < node->key_count; ++i) {
            bt_debug_printf("%d ", node->keys[i].id);
        }
        bt_debug_printf("\n");

        if (bt_is_node_leaf(node)) {
            if (stack) {
                node = stack->node->subs[++stack->key_index];
                stack = stack->next;
            } else {
                break;
            }
        } else {
            BTreeStackFrame *frame = x_arena_push_struct(arena, BTreeStackFrame);
            frame->next = stack;
            stack = frame;
            frame->node = node;
            frame->key_index = 0;
            node = node->subs[0];
        }
    }

    x_arena_temp_end(temp);
}

#endif /* BTREE_IMPLEMENTATION */
