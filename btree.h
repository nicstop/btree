#ifndef BTREE_HEADER_INCLUDE
#define BTREE_HEADER_INCLUDE

/* NOTE(nick): Here you can pick which order you want for this tree. */
#define BTREE_ORDER 2
#define BTREE_NODE_COUNT (BTREE_ORDER * 2)
#define BTREE_KEY_COUNT (BTREE_NODE_COUNT - 1)

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
    return tree->allocator.alloc_memory(tree->allocator.alloc_memory_context, sizeof(BTreeNode));
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
bt_shift_keys_right(BTreeNode *node, U32 start_key)
{
    S32 i;
    for (i = x_countof(node->keys) - 2; i >= 0; --i) {
        if (i < (S32)start_key) {
            break;
        }
        node->keys[i + 1] = node->keys[i];
        node->keys[i].id = BTREE_INVALID_ID;
        node->keys[i].data = NULL;
        node->keys[i].data_size = 0;
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
        U32 i = 0;
        BTreeNode *node_sub = node->subs[node->key_count];

        for (i = 0; i < node->key_count; ++i) {
            if (node->keys[i].id == id) {
                return &node->keys[i];
            }

            if (id < node->keys[i].id) {
                node_sub = node->subs[i];
                break;
            }
        }

        if (node_sub == NULL) {
            break;
        }
        node = node_sub;
    }

    return NULL;
}

BTREE_API void
bt_insert(BTree *tree, U32 id, void *data, U32 data_size)
{
    bt_reset_stack(tree);

    {
        BTreeNode *node;

        if (tree->root == NULL) {
            tree->root = bt_new_node(tree);
        }
        node = tree->root;

        while (node) {
            U32 key_index;

            for (key_index = 0; key_index < node->key_count; ++key_index) {
                if (node->keys[key_index].id == id) {
                    return;
                } else if (id < node->keys[key_index].id) {
                    break;
                }
            }

            /* NOTE(nick): Pushing frame in case we need to split. */
            bt_push_stack_frame(tree, node, key_index);

            if (node->subs[key_index] == NULL) {
                x_assert(node->key_count < x_countof(node->keys));
                bt_shift_keys_right(node, key_index);
                node->keys[key_index].id = id;
                node->keys[key_index].data = data;
                node->keys[key_index].data_size = data_size;
                node->key_count += 1;

                node = NULL;
            } else {
                node = node->subs[key_index];
            }
        }
    }

    {
        BTreeStackFrame frame;
        BTreeStackFrame frame_parent;

        while (bt_pop_stack_frame(tree, &frame)) {
            BTreeNode *node_split = NULL;
            BTreeKey *median_key = NULL;
            U32 i;

            if (frame.node->key_count < x_countof(frame.node->keys)) {
                break;
            }

            node_split = bt_new_node(tree);

            /* NOTE(nick): Copy upper-half of the sub-nodes to the split node. */
            for (i = x_countof(frame.node->subs) / 2; i < x_countof(frame.node->subs); ++i) {
                node_split->subs[i - x_countof(frame.node->subs) / 2] = frame.node->subs[i];
                frame.node->subs[i] = NULL;
            }

            for (i = 0; i < x_countof(frame.node->keys) / 2; ++i) {
                BTreeKey *key_a = &node_split->keys[node_split->key_count++];
                BTreeKey *key_b = &frame.node->keys[--frame.node->key_count];
                *key_a = *key_b;
                bt_memset(key_b, 0, sizeof(*key_b));
                key_b->id = BTREE_INVALID_ID;
            }

            median_key = &frame.node->keys[--frame.node->key_count];

            if (bt_peek_stack_frame(tree, &frame_parent)) {
                S32 k;

                x_assert(frame_parent.node->key_count < x_countof(frame_parent.node->keys));

                bt_shift_keys_right(frame_parent.node, frame_parent.key_index);
                frame_parent.node->keys[frame_parent.key_index] = *median_key;
                bt_memset(median_key, 0, sizeof(*median_key));
                frame_parent.node->key_count += 1;

                frame_parent.key_index += 1;
                for (k = frame_parent.node->key_count; k >= (S32)frame_parent.key_index; --k) {
                    frame_parent.node->subs[k + 1] = frame_parent.node->subs[k];
                    frame_parent.node->subs[k] = NULL;
                }

                x_assert(frame_parent.node->subs[frame_parent.key_index] == NULL);
                frame_parent.node->subs[frame_parent.key_index] = node_split;
            } else {
                /* NOTE(nick): Splitting reached root node, increasing height of the tree. */
                BTreeNode *new_root = bt_new_node(tree);
                new_root->key_count = 1;
                new_root->keys[0] = *median_key;
                new_root->subs[0] = frame.node;
                new_root->subs[1] = node_split;
                tree->root = new_root;
                bt_memset(median_key, 0, sizeof(*median_key));
            }
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
