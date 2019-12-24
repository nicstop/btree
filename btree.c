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
bt_shift_keys_right(BTreeNode *node, U32 start_key)
{
    S32 i;

    for (i = x_countof(node->keys) - 2; i >= 0; --i) {
        node->keys[i + 1] = node->keys[i];
        node->keys[i].id = BTREE_INVALID_ID;
        node->keys[i].data = NULL;
        node->keys[i].data_size = 0;

        if (i == (S32)start_key) {
            break;
        }
    }
}

BTreeKey *
bt_search(BTree *tree, U32 id)
{
    BTreeNode *node = tree->root;

    while (node) {
        U32 i = 0;
        BTreeNode *node_sub = node->subs[node->next_key_index];

        for (i = 0; i < node->next_key_index; ++i) {
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


        {
            BTreeStackFrame *frame = x_arena_push_struct(arena, BTreeStackFrame);
            frame->node = node;
            frame->key_index = key_index;
            frame->next = stack;
            stack = frame;
        }

        if (node->subs[key_index] != NULL) {
            node = node->subs[key_index];
            continue;
        }

        if (node->next_key_index >= x_countof(node->keys)) {
            if (bt_is_node_leaf(node)) {
                BTreeStackFrame *frame = stack;
                BTreeStackFrame *frame_prev = 0;

                BTreeKey *median_key = NULL;
                BTreeNode *node_split = NULL;

                while (frame) {
                    node_split = bt_new_node(arena);

                    for (i = 0; i < x_countof(node->keys) / 2; ++i) {
                        U32 k = x_countof(node->keys) / 2 + i + 1;
                        x_assert(k < x_countof(node->keys));

                        node_split->subs[i] = node->subs[k];
                        node_split->subs[i] = NULL;

                        node_split->keys[i] = node->keys[k];

                        node->keys[k].id = BTREE_INVALID_ID;
                        node->keys[k].data = 0;
                        node->keys[k].data_size = 0;
                    }
                    
                    node->next_key_index = i;
                    node_split->next_key_index = i;

                    median_key = &node->keys[i];

                    if (frame->next) {
                        BTreeStackFrame *frame_parent = frame->next;
                        BTreeNode *node_parent = frame_parent->node;
                        U32 node_split_key_index = frame_parent->key_index;

                        if (node_parent->next_key_index < x_countof(node_parent->keys)) {
                            S32 k;

                            bt_shift_keys_right(node_parent, node_split_key_index);

                            node_parent->keys[node_split_key_index] = *median_key;
                            x_memset(median_key, 0, sizeof(*median_key));

                            node_split_key_index += 1;
                            for (k = node_parent->next_key_index; k > (S32)node_split_key_index; --k) {
                                node_parent->subs[k + 1] = node_parent->subs[k];
                                node_parent->subs[k] = NULL;
                            }

                            x_assert(node_parent->subs[node_split_key_index] == NULL);
                            node_parent->subs[node_split_key_index] = node_split;
                        } else {
                            int x = 0;
                        }
                    } else {
                        /* NOTE(nick): Splitting reached root node, increasing height of the tree. */
                        BTreeNode *new_root = bt_new_node(arena);
                        new_root->next_key_index = 1;
                        new_root->keys[0] = *median_key;
                        new_root->subs[0] = node;
                        new_root->subs[1] = node_split;
                        tree->root = new_root;

                        x_memset(median_key, 0, sizeof(*median_key));
                        return;
                    }

                    frame_prev = frame;
                    frame = frame->next;
                }
            } else {
                node = node->subs[key_index];
            }
        }

        bt_shift_keys_right(node, key_index);

        node->keys[key_index].id = id;
        node->keys[key_index].data = data;
        node->keys[key_index].data_size = data_size;
        node->next_key_index += 1;

        break;
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

        for (i = 0; i < node->next_key_index; ++i) {
            printf("%d ", node->keys[i].id);
        }
        printf("\n");

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
