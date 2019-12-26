#ifndef BTREE_HEADER_INCLUDE
#define BTREE_HEADER_INCLUDE

#define BTREE_ORDER 2
#define BTREE_NODE_COUNT (BTREE_ORDER * 2)
#define BTREE_KEY_COUNT (BTREE_NODE_COUNT - 1)

#define U32 unsigned int

#define BTREE_INVALID_ID UINT32_MAX
typedef int BTreeKeyID;

#ifndef BTREE_CUSTOM_MEM
    #include <string.h>
    #define bt_memcopy(dst, src, size)  memcpy(dst, src, size)
    #define bt_memmove(dst, src, size)  memcpy(dst, src, size); memset(src, 0, size)
    #define bt_memset(ptr, value, size) memset(ptr, value, size)
#endif

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

typedef struct BTree {
    BTreeNode *root;

    U32 height;
} BTree;

typedef struct BTreeStackFrame {
    BTreeNode *node;
    U32 key_index;
    struct BTreeStackFrame *next;
} BTreeStackFrame;

#endif /* BTREE_HEADER_INCLUDE */

#ifdef BTREE_IMPLEMENTATION

#if 0

BTreeKey *
bt_find_mid(BTreeKey *start, BTreeKey *last)
{
    BTreeKey *result = NULL;

    if (start != NULL) {
        BTreeKey *slow = start;
        BTreeKey *fast = start->next;

        while (fast != last) {
            fast = fast->next;
            if (fast != last) {
                slow = slow->next;
                fast = fast->next;
            }
        }

        result = slow;
    }

    return result;
} 

BTreeKey *
bt_search_keys(BTreeKey *head, U32 id)
{
    BTreeKey *start = head;
    BTreeKey *last = NULL;
    
    do {
        BTreeKey *mid = bt_find_mid(start, last);

        if (mid == NULL) {
            break;
        }

        if (mid->id == id) {
            return mid;
        } else if (mid->id < id) {
            start = mid->next;
        } else {
            last = mid;
        }
    } while (last == NULL || last != start);

    return NULL;
}

#endif

void
bt_debug_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

BTreeNode *
bt_new_node(x_arena *memory)
{
    BTreeNode *node = x_arena_push_struct(memory, BTreeNode);

    x_memset(&node->keys[0], 0, sizeof(node->keys));
    x_memset(&node->subs[0], 0, sizeof(node->subs));

    return node;
}

xbool
bt_is_node_leaf(BTreeNode *node)
{
    U32 i;
    for (i = 0; i < x_countof(node->subs); ++i) {
        if (node->subs[i] != 0) {
            return false;
        }
    }
    return true;
}

void
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

BTreeKey *
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

void
bt_insert(BTree *tree, U32 id, void *data, U32 data_size, x_arena *arena)
{
    BTreeNode *node;
    BTreeStackFrame *stack = 0;

    if (tree->root == NULL) {
        tree->root = bt_new_node(arena);
    }
    node = tree->root;

    while (node) {
        U32 i;
        U32 key_index;

        for (key_index = 0; key_index < node->key_count; ++key_index) {
            if (node->keys[i].id == id) {
                return;
            } else if (id < node->keys[key_index].id) {
                break;
            }
        }

        {
            BTreeStackFrame *frame = x_arena_push_struct(arena, BTreeStackFrame);
            frame->node = node;
            frame->key_index = key_index;
            frame->next = stack;
            stack = frame;
        }

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

#if 0
    bt_debug_printf("Associating data with key-index %d\n", key_index);
    bt_debug_printf("Keys: ");
    for (i = 0; i < node->key_count; ++i) {
        bt_debug_printf("%d ", node->keys[i].id);
    }
    bt_debug_printf("\n");
#endif

    while (stack) {
        BTreeKey *median_key = NULL;
        BTreeNode *node_split = NULL;
        U32 i;

        node = stack->node;

        if (node->key_count < x_countof(node->keys)) {
            break;
        }

        node_split = bt_new_node(arena);

        /* NOTE(nick): Copy upper-half of the sub-nodes to the split node. */
        for (i = x_countof(node->subs) / 2; i < x_countof(node->subs); ++i) {
            node_split->subs[i - x_countof(node->subs) / 2] = node->subs[i];
            node->subs[i] = NULL;
        }

        for (i = 0; i < x_countof(node->keys) / 2; ++i) {
            BTreeKey *key_a = &node_split->keys[node_split->key_count++];
            BTreeKey *key_b = &node->keys[--node->key_count];

            *key_a = *key_b;
            x_memset(key_b, 0, sizeof(*key_b));
            key_b->id = BTREE_INVALID_ID;
        }

        median_key = &node->keys[--node->key_count];

        if (stack->next) {
            BTreeStackFrame *frame_parent = stack->next;
            BTreeNode *node_parent = frame_parent->node;
            S32 k;

            x_assert(node_parent->key_count < x_countof(node_parent->keys));

            bt_shift_keys_right(node_parent, frame_parent->key_index);
            node_parent->keys[frame_parent->key_index] = *median_key;
            x_memset(median_key, 0, sizeof(*median_key));
            node_parent->key_count += 1;

            frame_parent->key_index += 1;
            for (k = node_parent->key_count; k >= (S32)frame_parent->key_index; --k) {
                node_parent->subs[k + 1] = node_parent->subs[k];
                node_parent->subs[k] = NULL;
            }

            x_assert(node_parent->subs[frame_parent->key_index] == NULL);
            node_parent->subs[frame_parent->key_index] = node_split;
        } else {
            /* NOTE(nick): Splitting reached root node, increasing height of the tree. */
            BTreeNode *new_root = bt_new_node(arena);
            new_root->key_count = 1;
            new_root->keys[0] = *median_key;
            new_root->subs[0] = node;
            new_root->subs[1] = node_split;
            tree->root = new_root;
            x_memset(median_key, 0, sizeof(*median_key));
        }

        stack = stack->next;
    }
}

void
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
