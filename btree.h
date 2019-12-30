#ifndef BTREE_HEADER_INCLUDE
#define BTREE_HEADER_INCLUDE

/* 
 * B-Tree single-header lib.
 * -------------------------------------------------------------------------------- 
 * Nikita Smith 
 */

/* NOTE(nick): Here you can pick which order you want for this tree. */
#define BTREE_NODE_COUNT 4 
#define BTREE_KEY_COUNT ((BTREE_NODE_COUNT) - 1) 

#define BTREE_API
#define BTREE_INTERNAL

typedef unsigned int U32;

#define bt_false 0
#define bt_true 1
typedef int bt_bool;

#define BTREE_INVALID_ID UINT32_MAX
typedef int BTreeKeyID;

#ifndef BTREE_CUSTOM_MEM
    #include <string.h>
    #define bt_memcopy(dst, src, size)  memcpy(dst, src, size)
    #define bt_memmove(dst, src, size)  memcpy(dst, src, size); memset(src, 0, size)
    #define bt_memset(ptr, value, size) memset(ptr, value, size)
#endif

#define BTREE_ASSERT(cnd) if (!(cnd)) { *((volatile int *)0); }

#define BTREE_MALLOC_SIG(name) void * name(void *user_context, size_t size)
typedef BTREE_MALLOC_SIG(bt_malloc_sig);

#define BTREE_FREE_SIG(name) void name(void *user_context, void *ptr)
typedef BTREE_FREE_SIG(bt_free_sig);

#define BTREE_REALLOC_SIG(name) void * name(void *user_context, void *ptr, size_t size)
typedef BTREE_REALLOC_SIG(bt_realloc_sig);

#define BTREE_VISIT_NODES_SIG(name) bt_bool name(void *user_context, BTreeKeyID id, void *data, U32 data_size)
typedef BTREE_VISIT_NODES_SIG(bt_visit_nodes_sig);

typedef struct BTreeKey {
    BTreeKeyID id;
    void *data;
    U32 data_size;
} BTreeKey;

typedef struct BTreeNode {
    U32 key_count;
    BTreeKey keys[BTREE_KEY_COUNT];
    struct BTreeNode *subs[BTREE_NODE_COUNT];
} BTreeNode;

typedef struct BTreeStackFrame {
    BTreeNode *node;
    U32 key_index;
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

    U32 stack_push_size;
    U32 stack_memory_size;
    void *stack_memory;

    BTreeNode *root;
    U32 height;
    BTreeStackFrame *stack;
} BTree;

BTREE_API void
bt_create(BTree *tree, BTreeAllocator *allocator);

BTREE_API void
bt_destroy(BTree *tree);

BTREE_API BTreeKey *
bt_search(BTree *tree, U32 id);

BTREE_API void
bt_insert(BTree *tree, U32 id, void *data, U32 data_size);

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
bt_shift_keys_right(BTreeNode *node, U32 start_key);

BTREE_INTERNAL void
bt_push_stack_frame(BTree *tree, BTreeNode *node, BTreeKeyID key_id);

BTREE_INTERNAL bt_bool
bt_pop_stack_frame(BTree *tree, BTreeStackFrame *frame_out);

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
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
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
bt_node_set_key(BTreeNode *node, U32 key_index, BTreeKeyID id, void *data, U32 data_size)
{
    x_assert(key_index < x_countof(node->keys));
    node->keys[key_index].id = id;
    node->keys[key_index].data = data;
    node->keys[key_index].data_size = data_size;
}

BTREE_INTERNAL BTreeKey *
bt_node_get_key(BTreeNode *node, U32 key_index)
{
    x_assert(key_index < x_countof(node->keys));
    return &node->keys[key_index];
}

BTREE_INTERNAL void
bt_node_invalidate_key(BTreeNode *node, U32 key_index)
{
    x_assert(key_index < x_countof(node->keys));
    bt_node_set_key(node, key_index, BTREE_INVALID_ID, NULL, 0);
}

BTREE_INTERNAL void
bt_node_add_key(BTreeNode *node, BTreeKeyID id, void *data, U32 data_size)
{
    if (node->key_count < x_countof(node->keys)) {
        bt_node_set_key(node, node->key_count, id, data, data_size);
        node->key_count += 1;
    } else {
        x_assert_msg("cannot add key to a full node");
    }
}

BTREE_INTERNAL void
bt_node_remove_key(BTreeNode *node)
{
    if (node->key_count > 0) {
        bt_node_invalidate_key(node, node->key_count - 1);
        node->key_count -= 1;
    } else {
        x_assert_msg("no keys to remove from an empty node");
    }
}

BTREE_INTERNAL bt_bool
bt_node_set_sub(BTreeNode *node, U32 sub_index, BTreeNode *sub)
{
    bt_bool result = bt_false;

    if (sub_index < x_countof(node->subs)) {
        node->subs[sub_index] = sub;
        result = bt_true;
    } else {
        x_assert_msg("sub index out of bounds");
    }

    return result;
}

BTREE_INTERNAL BTreeNode *
bt_node_get_sub(BTreeNode *node, U32 sub_index)
{
    BTreeNode *result;

    if (sub_index < x_countof(node->subs)) {
        result = node->subs[sub_index];
    } else {
        x_assert("sub index out of bounds, returning NULL");
        result = NULL;
    }

    return result;
}

BTREE_INTERNAL bt_bool
bt_is_node_leaf(BTreeNode *node)
{
    U32 i;
    for (i = 0; i < x_countof(node->subs); ++i) {
        if (node->subs[i] != 0) {
            return bt_false;
        }
    }
    return bt_true;
}

BTREE_INTERNAL void
bt_shift_keys_left(BTreeNode *node, U32 key_index)
{
    U32 i;
    x_assert(node->key_count > 0);
    for (i = key_index; i < node->key_count - 1; ++i) {
        BTreeKey *key;

        key = bt_node_get_key(node, i + 1);
        bt_node_set_key(node, i, key->id, key->data, key->data_size);
        bt_node_invalidate_key(node, i + 1);
    }
}

BTREE_INTERNAL void
bt_shift_keys_right(BTreeNode *node, U32 start_key)
{
    S32 i;
    for (i = x_countof(node->keys) - 2; i >= 0; --i) {
        BTreeKey *key;

        if (i < (S32)start_key) {
            break;
        }
        key = bt_node_get_key(node, i);
        bt_node_set_key(node, i + 1, key->id, key->data, key->data_size);
        bt_node_invalidate_key(node, i);
    }
}

BTREE_INTERNAL void
bt_shift_subs_left(BTreeNode *node, U32 key_index)
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
bt_shift_subs_right(BTreeNode *node, U32 key_index)
{
    S32 i;
    for (i = (S32)node->key_count - 1; i >= (S32)key_index; --i) {
        BTreeNode *sub;

        sub = bt_node_get_sub(node, i);
        bt_node_set_sub(node, i + 1, sub);
        bt_node_set_sub(node, i, NULL);
    }
}

BTREE_INTERNAL void
bt_push_stack_frame(BTree *tree, BTreeNode *node, BTreeKeyID key_id)
{
    BTreeStackFrame *frame;

    if (tree->stack_memory == NULL) {
        tree->stack_memory_size = 128 * sizeof(BTreeStackFrame);
        tree->stack_memory = bt_malloc(tree, tree->stack_memory_size);
        tree->stack_push_size = 0;
        BTREE_ASSERT(tree->stack_memory != NULL);
    }

    if (tree->stack_push_size + sizeof(BTreeStackFrame) > tree->stack_memory_size) {
        tree->stack_memory_size = tree->stack_memory_size * 2;
        tree->stack_memory = bt_realloc(tree, tree->stack_memory, tree->stack_memory_size);
    }

    frame = (BTreeStackFrame *)((char *)tree->stack_memory + tree->stack_push_size);
    frame->node = node;
    frame->key_index = key_id;
    frame->next = tree->stack;
    tree->stack = frame;

    tree->stack_push_size += sizeof(BTreeStackFrame);
}

BTREE_INTERNAL bt_bool
bt_pop_stack_frame(BTree *tree, BTreeStackFrame *frame_out)
{
    bt_bool result = bt_false;
    if (tree->stack) {
        *frame_out = *tree->stack;
        tree->stack = frame_out->next;
        result = bt_true;
    }
    return result;
}

BTREE_INTERNAL bt_bool
bt_peek_stack_frame(BTree *tree, BTreeStackFrame *frame_out)
{
    bt_bool result = bt_false;
    if (tree->stack) {
        *frame_out = *tree->stack;
        result = bt_true;
    }
    return result;
}

BTREE_INTERNAL void
bt_reset_stack(BTree *tree)
{
    tree->stack_push_size = 0;
    tree->stack = NULL;
}

BTREE_API void
bt_create(BTree *tree, BTreeAllocator *allocator)
{
    tree->root = NULL;
    tree->height = 0;

    tree->stack_push_size = 0;
    tree->stack_memory_size = 0;
    tree->stack_memory = NULL;

    tree->allocator = *allocator;
}

BTREE_API void
bt_destroy(BTree *tree)
{
    /* TODO(nick): IMPLEMENT */
}

BTREE_API BTreeKey *
bt_search(BTree *tree, U32 id)
{
    BTreeNode *node = tree->root;

    while (node) {
#if 1
        U32 key_index;

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
        U32 min, max, mid;

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

    return NULL;
}

BTREE_API void
bt_insert(BTree *tree, U32 id, void *data, U32 data_size)
{
    BTreeStackFrame frame;

    if (!tree->root) {
        tree->root = bt_new_node(tree);
    }

    {
        BTreeNode *node = tree->root;

        bt_reset_stack(tree);
        while (node) {
            U32 key_index;

            for (key_index = 0; key_index < node->key_count; ++key_index) {
                if (node->keys[key_index].id == id) {
                    break;
                } else if (id < node->keys[key_index].id) {
                    break;
                }
            }

            /* NOTE(nick): Pushing frame in case we need to split. */
            bt_push_stack_frame(tree, node, key_index);

            if (node->subs[key_index] != NULL) {
                node = node->subs[key_index];
            } else {
                node = NULL;
            }
        }
    }

#if 0
        bt_dump_stack(tree);
#endif

    if (bt_peek_stack_frame(tree, &frame)) {
        if (frame.node->keys[frame.key_index].id != id) {
            x_assert(frame.node->key_count < x_countof(frame.node->keys));
            bt_shift_keys_right(frame.node, frame.key_index);
            frame.node->key_count += 1;
            bt_node_set_key(frame.node, frame.key_index, id, data, data_size);
        }
    }

    {
        while (bt_pop_stack_frame(tree, &frame)) {
            BTreeNode *node_split = NULL;
            BTreeKey median_key;
            U32 i;

            if (frame.node->key_count < x_countof(frame.node->keys)) {
                break;
            }

            node_split = bt_new_node(tree);

            bt_debug_printf("--------------------------------------------------------------------------------\n");
            bt_debug_printf("Detected a full node, splitting keys:\n");
            bt_dump_node_keys(frame.node);

            /* NOTE(nick): Copy upper-half of the sub-nodes to the split node. */
            for (i = x_countof(frame.node->subs) / 2; i < x_countof(frame.node->subs); ++i) {
                BTreeNode *sub = bt_node_get_sub(frame.node, i);
                bt_node_set_sub(node_split, i - x_countof(node_split->subs) / 2, sub);
                bt_node_set_sub(frame.node, i, NULL);
            }

            /* NOTE(nick): Copy upper-half of the keys to the split node. */
            for (i = x_countof(frame.node->keys) / 2; i < x_countof(frame.node->keys); ++i) {
                BTreeKey *key = bt_node_get_key(frame.node, i);
                bt_node_add_key(node_split, key->id, key->data, key->data_size);
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
            x_assert(node_split->key_count > 0);
            x_assert(frame.node->key_count > 0);

            bt_debug_printf("Median: %d\n", median_key.id);

            { 
                /* NOTE(nick): Inserting a new median key. */

                BTreeStackFrame frame_parent;

                if (bt_peek_stack_frame(tree, &frame_parent)) {

                    x_assert(frame_parent.node->key_count < x_countof(frame_parent.node->keys));
                    bt_shift_keys_right(frame_parent.node, frame_parent.key_index);
                    bt_node_set_key(frame_parent.node, frame_parent.key_index, median_key.id, median_key.data, median_key.data_size);

                    frame_parent.node->key_count += 1;
                    frame_parent.key_index += 1;

                    bt_shift_subs_right(frame_parent.node, frame_parent.key_index);
                    x_assert(bt_node_get_sub(frame_parent.node, frame_parent.key_index) == NULL);
                    bt_node_set_sub(frame_parent.node, frame_parent.key_index, node_split);
                } else {
                    /* NOTE(nick): Splitting reached root node, inserting a new root. */
                    BTreeNode *new_root = bt_new_node(tree);
                    bt_node_add_key(new_root, median_key.id, median_key.data, median_key.data_size);
                    bt_node_set_sub(new_root, 0, frame.node);
                    bt_node_set_sub(new_root, 1, node_split);
                    tree->root = new_root;
                }
            }
        }
    }
}

BTREE_INTERNAL bt_bool
bt_delete(BTree *tree, U32 id)
{
    bt_bool found = bt_false;

    BTreeNode *node = tree->root;

    U32 key_index_delete = BTREE_INVALID_ID;
    BTreeNode *node_delete = NULL;

    BTreeNode *node_new_separator = NULL;

    bt_reset_stack(tree);

    while (node && node_delete == NULL) {
        U32 key_index;

        for (key_index = 0; key_index < node->key_count; ++key_index) {
            if (id == node->keys[key_index].id) {
                node_delete = node;
                key_index_delete = key_index;
                break;
            } else if (id < node->keys[key_index].id) {
                break;
            }
        }

        bt_push_stack_frame(tree, node, key_index);
        node = node->subs[key_index];
    }

    {
        /* NOTE(nick): Picking largest ID on the left branch of node_delete. */

        BTreeStackFrame *frame_with_separator = NULL;

        while (node != NULL) {
            bt_push_stack_frame(tree, node, node->key_count);

            if (node_new_separator == NULL) {
                node_new_separator = node;
                frame_with_separator = tree->stack;
            }

            if (node->keys[node->key_count - 1].id > node_new_separator->keys[node_new_separator->key_count - 1].id) {
                node_new_separator = node;
                frame_with_separator = tree->stack;
            }

            node = node->subs[node->key_count];
        }

        if (frame_with_separator != NULL) {
            if (frame_with_separator->node->subs[frame_with_separator->key_index] == NULL) {
                frame_with_separator->key_index = frame_with_separator->node->key_count - 1;
            }
        }

        tree->stack = frame_with_separator;

        /* bt_dump_stack(tree); */
    }

    if (node_new_separator != NULL) {
        /* NOTE(nick): Replacing deleted key with a new separator. Also, there is no
         * need to shift keys to left since it is a last key. */
        node_delete->keys[key_index_delete] = node_new_separator->keys[node_new_separator->key_count - 1];
        bt_node_invalidate_key(node_new_separator, node_new_separator->key_count - 1);
        node_new_separator->key_count -= 1;
    } else {
        bt_shift_keys_left(node_delete, key_index_delete);
        node_delete->key_count -= 1;
    }

    /* NOTE(nick): Rebalancing after deletion. */
    while (true) {
        BTreeStackFrame frame, frame_parent;

        BTreeNode *node_separator;
        U32 key_index_separator;

        BTreeNode *node_right = NULL;
        BTreeNode *node_left = NULL;
        
        if (!bt_pop_stack_frame(tree, &frame)) {
            break;
        }
        if (!bt_peek_stack_frame(tree, &frame_parent)) {
            /* TODO(nick): IMPLEMENT */
            break;
        }

        if (frame.node->key_count >  0) {
            break;
        }

        node_separator = frame_parent.node;
        key_index_separator = frame_parent.key_index;


        if (frame_parent.key_index > 0) {
            node_left = frame_parent.node->subs[frame_parent.key_index - 1];
        }
        /* NOTE(nick): This is a weird hack that is needed when deleting right-most.  */
        if (key_index_separator >= node_separator->key_count) {
            key_index_separator -= 1;
        }
        if ((key_index_separator + 1) < x_countof(frame_parent.node->subs)) {
            node_right = frame_parent.node->subs[key_index_separator + 1];
        } 

        if (node_right != NULL && node_right->key_count > 1) {
            x_assert(frame.node->subs[2] == NULL);

            frame.node->keys[0] = frame_parent.node->keys[frame_parent.key_index];
            frame.node->subs[1] = node_right->subs[0];
            frame.node->key_count += 1;

            frame_parent.node->keys[frame_parent.key_index] = node_right->keys[0];

            bt_shift_subs_left(node_right, 0);
            bt_shift_keys_left(node_right, 0);

            node_right->key_count -= 1;
        } else if (node_left != NULL && node_left->key_count > 1) {
            frame.node->keys[0] = frame_parent.node->keys[frame_parent.key_index - 1];
            frame.node->key_count = 1;

            bt_shift_subs_right(frame.node, 0);
            frame.node->subs[0] = node_left->subs[node_left->key_count];
            node_left->subs[node_left->key_count] = NULL;

            frame_parent.node->keys[frame_parent.key_index - 1] = node_left->keys[node_left->key_count - 1];
            bt_node_invalidate_key(node_left, node_left->key_count - 1);
            node_left->key_count -= 1;
        } else {
            BTreeNode *node_container;
            U32 i;

            if (node_left != NULL) {
                x_assert(node_left->key_count < x_countof(node_left->keys));
                x_assert(frame_parent.key_index > 0 );
                node_container = node_left;

                node_container->keys[node_container->key_count++] = node_separator->keys[frame_parent.key_index - 1];
                bt_shift_keys_left(node_separator, frame_parent.key_index - 1);
                bt_shift_subs_left(node_separator, frame_parent.key_index);
            } else {
                node_container = frame.node;
                x_assert(node_container->key_count == 0);

                node_container->keys[node_container->key_count++] = node_separator->keys[key_index_separator];
                bt_shift_keys_left(node_separator, key_index_separator);
            }

            if (node_right != NULL) {
                S32 i;
                S32 copy_count = (S32)node_right->key_count;

                if (node_container->key_count + copy_count >= x_countof(node_container->keys)) {
                    copy_count = x_countof(node_container->keys) - node_right->key_count - node_container->key_count - 1;
                }

                if (copy_count >= 0) {
                    for (i = 0; i < copy_count; ++i) {
                        x_assert(node_container->key_count < x_countof(node_container->keys));
                        node_container->keys[node_container->key_count] = node_right->keys[i];
                        node_container->subs[node_container->key_count] = node_right->subs[i];

                        bt_node_invalidate_key(node_right, i);
                        bt_shift_subs_left(node_right, i);

                        node_container->key_count += 1;
                        node_right->key_count -= 1;
                    }

                    node_container->subs[node_container->key_count] = node_right->subs[node_right->key_count];
                    node_right->subs[node_right->key_count] = NULL;

                    if (node_right->key_count == 0) {
                        bt_shift_subs_left(node_separator, key_index_separator + 1);
                        bt_free(tree, node_right);
                    }
                }
            }

            bt_node_invalidate_key(node_separator, node_separator->key_count - 1);
            node_separator->key_count -= 1;

            if (frame_parent.node == tree->root) {
                if (frame_parent.node->key_count == 0) {
                    bt_free(tree, tree->root);
                    tree->root = node_container;
                }
            }
        }
    }

    return found;
}

BTREE_API void
bt_visit_nodes(BTree *tree, void *user_context, bt_visit_nodes_sig *visit)
{
    BTreeNode *node = tree->root;

    if (node != NULL) {
        bt_reset_stack(tree);
        while (node != NULL) {
            U32 i;

            for (i = 0; i < node->key_count; ++i) {
                if (visit(user_context, node->keys[i].id, node->keys[i].data, node->keys[i].data_size) == bt_false) {
                    return;
                }
            }

            if (bt_is_node_leaf(node)) {
                BTreeStackFrame frame;

                node = NULL;
                while (bt_pop_stack_frame(tree, &frame)) {
                    frame.key_index += 1;
                    if (frame.key_index <= frame.node->key_count) {
                        if (frame.node->subs[frame.key_index] != NULL) {
                            bt_push_stack_frame(tree, frame.node, frame.key_index);
                            node = frame.node->subs[frame.key_index];
                            x_assert(node->key_count > 0);
                            break;
                        }
                    }
                }
            } else {
                x_assert(node->key_count > 0);
                bt_push_stack_frame(tree, node, 0);
                node = node->subs[0];
            }
        }
    }
}

BTREE_INTERNAL void
bt_dump_stack(BTree *tree)
{
    BTreeStackFrame *frame = tree->stack;
    U32 depth = 0;
    bt_debug_printf("Dumping node stack:\n");
    while (frame) {
        if (frame->key_index < x_countof(frame->node->keys)) {
            bt_debug_printf("%d: key_index: %d, id: %d\n", depth, frame->key_index, frame->node->keys[frame->key_index].id);
        } else {
            bt_debug_printf("%d: key_index: %d, no id\n", depth, frame->key_index);
        }
        depth += 1;
        frame = frame->next;
    }
}

BTREE_INTERNAL void
bt_dump_node_keys(BTreeNode *node)
{
    U32 i;
    for (i = 0; i < node->key_count; ++i) {
        bt_debug_printf("%d ", node->keys[i].id);
    }
    bt_debug_printf("\n");

    for (i = 0; i < x_countof(node->subs); ++i) {
        if (node->subs[i] != NULL) {
            U32 k;
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
    U32 depth = 0;
    x_arena_temp temp;
    BTreeStackFrame *stack = 0;

    temp = x_arena_temp_begin(arena);

    while (node) {
        U32 i;

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
