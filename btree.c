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
bt_split_node(BTreeNode *node, BTreeStackFrame *stack, x_arena *arena)
{
    BTreeKeyID median_index = node->keys[x_countof(node->keys) / 2].id;
    BTreeStackFrame *frame = stack;
    BTreeStackFrame *frame_prev = 0;

    BTreeKey *median = 0;
    U32 median_key = BTREE_INVALID_ID;
    BTreeNode *node_split = 0;
    while (frame) {
        node_split = bt_new_node(arena);
        while (node->next_key_index > x_countof(node->keys) / 2) {
            node->next_key_index -= 1;
            node_split->keys[node_split->next_key_index] = node->keys[node->next_key_index];
            node_split->next_key_index += 1;
        }

#if 0
        {
            BTreeStackFrame *frame_parent = frame->next;

            if (parent_frame) {
                BTreeNode *node_parent = parent_frame->node;

                /* TODO(nick): write to parent new split node */

                node_parent->subs[frame_parent->node_index + 1] = node_split;
            } else {

            }
        }

        if (median) {
            if (node->next_key_index < x_countof(frame->node->keys)) {
                bt_shift_keys_right(node, median_key);
                node->subs[median_key] = ;
                node->subs[median_key] = node_split;
            }


        }
#endif

        median = &node_split->keys[node->next_key_index - 1];
        node->next_key_index -= 1;

        frame_prev = frame;
        frame = frame->next;
    }
}

void
bt_shift_keys_right(BTreeNode *node, U32 start_key)
{
    S32 i;

    x_assert(node->next_key_index < x_countof(node->keys));
    for (i = node->next_key_index; i >= (S32)start_key; --i) {
        node->keys[i + 1] = node->keys[i];
    }
    node->next_key_index += 1;

    for (i = node->next_key_index; i >= (S32)start_key; --i) {
        node->subs[i + 1] = node->subs[i];
    }
}

void
bt_insert(BTree *tree, U32 id, void *data, U32 data_size, x_arena *arena)
{
    BTreeNode *node;

    BTreeStackFrame *stack = 0;
    BTreeStackFrame *frame_prev = 0;
    BTreeStackFrame *frame = 0;

    BTreeKey *median = 0;
    U32 median_key = BTREE_INVALID_ID;
    BTreeNode *node_split = 0;
   
    if (tree->root == NULL) {
        tree->root = bt_new_node(arena);
    }


    node = tree->root;
    while (node) {
        U32 i;
        U32 key_index;

        for (key_index = 0; key_index < node->next_key_index; ++key_index) {
            if (node->keys[i].id == id) {
                return;
            } else if (id < node->keys[key_index].id) {
                break;
            }
        }



        if (node->next_key_index > x_countof(node->keys)) {
            if (bt_is_node_leaf(node)) {
                
            } else {
                BTreeStackFrame *frame = x_arena_push_struct(arena, BTreeStackFrame);
                frame->node = node;
                frame->key_index = key_index;
                frame->next = stack;
                stack = frame;

                node = node->subs[key_index];
            }
        }

        bt_shift_keys_right(node, key_index);

        node->keys[key_index].id = id;
        node->keys[key_index].data = data;
        node->keys[key_index].data_size = data_size;

        break;
    }
}

#endif /* BTREE_IMPLEMENTATION */
