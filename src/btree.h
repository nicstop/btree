#ifndef BT_HEADER_INCLUDE
#define BT_HEADER_INCLUDE

/* 
 * B-Tree single-header lib by Nikita Smith
 * ----------------------------------------
 */

#define BT_API
#define BT_INTERNAL static

#if 1
  #define BT_DEBUG_MODE
#endif

/*
 * Customize number of keys per node:
 */
#define BT_KEY_COUNT    (5)
#define BT_NODE_COUNT   (BT_KEY_COUNT + 1)

#ifndef BT_CUSTOM_DATA_TYPES

typedef unsigned char  bt_u08;
typedef signed   char  bt_s08;
typedef unsigned short bt_u16;
typedef signed   short bt_s16;
typedef unsigned long  bt_u32;
typedef signed long    bt_s32;
typedef unsigned long long bt_u64;
typedef signed   long long bt_s64;
#endif

typedef bt_s32         bt_bool;

#define bt_false 0
#define bt_true  1

#define BT_INVALID_ID 0xFFFFFFFFFFFFFFFF
typedef bt_u64 BT_KeyID;

#ifndef bt_memcpy
  #include <string.h>
  #define bt_memcpy memcpy
#endif

#ifndef bt_memset
  #include <string.h>
  #define bt_memset memset
#endif

#ifndef bt_malloc
  #include <stdlib.h>
  #define bt_malloc(size, ud) ((void)ud,malloc(size))
  #define bt_free(ptr, ud)    ((void)ud,free(ptr))
#endif

#if defined(__clang__)
#define BT_BREAK_TRAP_ __builtin_trap()
#else
#define BT_BREAK_TRAP_ (*((volatile int *)0))
#endif

#if defined(BT_DEBUG_MODE)
#define BT_BREAK_TRAP BT_BREAK_TRAP_
#else
#define BT_BREAK_TRAP
#endif

#ifndef BT_ASSERT
  #define BT_ASSERT(cnd)                  if (!(cnd)) { BT_BREAK_TRAP;  }
#endif

#ifndef BT_ASSERT_ALWAYS
  #define BT_ASSERT_ALWAYS(cnd)           if (!(cnd)) { BT_BREAK_TRAP_; }
#endif

#define BT_ASSERT_FAILURE(msg)          BT_ASSERT(0)
#define BT_ASSERT_FAILURE_ALWAYS(msg)   BT_ASSERT_ALWAYS(0)

#define BT_COUNTOF(x)                   (sizeof(x)/sizeof((x)[0]))

#define BT_VISIT_KEYS_SIG(name) bt_bool name(void *user_context, BT_KeyID id, const void *data)
typedef BT_VISIT_KEYS_SIG(bt_visit_keys_sig);

typedef enum {
  BT_ERROR_Ok,
  BT_ERROR_AllocationFailed,
  BT_ERROR_OpDenied,
  BT_ERROR_RebalanceFailed,
  BT_ERROR_IDNotFound
} BT_ErrorCode;

typedef enum {
  BT_VISIT_NODE_Null,
  BT_VISIT_NODE_TopDown,
  BT_VISIT_NODE_BottomUp
} BT_VisitNodesMode;

typedef struct BT_Key {
  BT_KeyID id;
  void const *data;
} BT_Key;

typedef struct BT_Node {
  bt_u08 key_count;
  BT_Key keys[BT_KEY_COUNT];
  struct BT_Node *subs[BT_NODE_COUNT];
} BT_Node;

typedef struct BT_StackFrame {
  bt_u08 key_index;
  BT_Node *node;
  struct BT_StackFrame *next;
} BT_StackFrame;

typedef struct BT_Context {
  void *malloc_ud;
  bt_u32 value_size;
  bt_u32 frames_count;
  bt_u32 frames_max;
  BT_StackFrame *frames;
  BT_Node *root;
} BT_Context;

BT_API BT_ErrorCode
bt_create(BT_Context *tree, bt_u32 value_size, void *malloc_ud);

BT_API BT_ErrorCode
bt_destroy(BT_Context *tree);

BT_API BT_Key *
bt_search(BT_Context *tree, BT_KeyID id, bt_bool get_nearest);

BT_API BT_ErrorCode
bt_insert(BT_Context *tree, BT_KeyID id, const void *data, bt_u32 data_size);

BT_API BT_ErrorCode
bt_delete(BT_Context *tree, BT_KeyID id);

BT_API BT_ErrorCode
bt_visit_keys(BT_Context *tree, BT_VisitNodesMode mode, void *user_context, bt_visit_keys_sig *visit);

/* -------------------------------------------------------------------------------- */

BT_INTERNAL BT_Node *
bt_new_node(BT_Context *tree);

BT_INTERNAL bt_bool
bt_is_node_leaf(BT_Node *node);

BT_INTERNAL void
bt_shift_keys_right(BT_Node *node, bt_u32 start_key);

BT_INTERNAL BT_ErrorCode
bt_push_stack_frame(BT_Context *tree, BT_Node *node, BT_KeyID key_id);

BT_INTERNAL BT_ErrorCode
bt_pop_stack_frame(BT_Context *tree, BT_StackFrame *frame_out);

BT_INTERNAL BT_ErrorCode
bt_peek_stack_frame(BT_Context *tree, BT_StackFrame *frame_out);

BT_INTERNAL void
bt_reset_stack(BT_Context *tree);

BT_INTERNAL void
bt_dump_stack(BT_Context *tree);

BT_INTERNAL void
bt_dump_node_keys(BT_Node *node);

#endif /* BT_HEADER_INCLUDE */

#ifdef BT_IMPLEMENTATION

BT_INTERNAL BT_Node *
bt_new_node(BT_Context *tree)
{
  BT_Node *node = (BT_Node *)bt_malloc(sizeof(BT_Node), tree->malloc_ud);
  if (node != NULL) {
#if 0
    bt_u32 i;
    for (i = 0; i < BT_COUNTOF(node->keys); ++i) {
      node->keys[i].id = BT_INVALID_ID;
      node->keys[i].data = 0;
    }
#else
    bt_memset(&node->keys[0], 0, sizeof(node->keys));
#endif
    node->key_count = 0;
    bt_memset(&node->subs[0], 0, sizeof(node->subs));
  }
  return node;
}

BT_INTERNAL void
bt_node_set_key(BT_Node *node, bt_u32 key_index, BT_KeyID id, const void *data)
{
  BT_ASSERT(key_index < BT_COUNTOF(node->keys));
  node->keys[key_index].id = id;
  node->keys[key_index].data = data;
}

BT_INTERNAL BT_Key *
bt_node_get_key(BT_Node *node, bt_u32 key_index)
{
  BT_ASSERT(key_index < BT_COUNTOF(node->keys));
  return &node->keys[key_index];
}

BT_INTERNAL void
bt_node_invalidate_key(BT_Node *node, bt_u32 key_index)
{
  BT_ASSERT(key_index < BT_COUNTOF(node->keys));
  bt_node_set_key(node, key_index, BT_INVALID_ID, NULL);
}

BT_INTERNAL void
bt_node_add_key(BT_Node *node, BT_KeyID id, const void *data)
{
  if (node->key_count < BT_COUNTOF(node->keys)) {
    bt_node_set_key(node, node->key_count, id, data);
    node->key_count += 1;
  } else {
    BT_ASSERT_FAILURE("cannot add key to a full node");
  }
}

BT_INTERNAL void
bt_node_remove_key(BT_Node *node)
{
  if (node->key_count > 0) {
    bt_node_invalidate_key(node, node->key_count - 1);
    node->key_count -= 1;
  } else {
    BT_ASSERT_FAILURE("no keys to remove from an empty node");
  }
}

BT_INTERNAL bt_bool
bt_node_set_sub(BT_Node *node, bt_u32 sub_index, BT_Node *sub)
{
  bt_bool result = bt_false;

  if (sub_index < BT_COUNTOF(node->subs)) {
    node->subs[sub_index] = sub;
    result = bt_true;
  } else {
    BT_ASSERT_FAILURE("sub index out of bounds");
  }

  return result;
}

BT_INTERNAL BT_Node *
bt_node_get_sub(BT_Node *node, bt_u32 sub_index)
{
  BT_Node *result;

  if (sub_index < BT_COUNTOF(node->subs)) {
    result = node->subs[sub_index];
  } else {
    BT_ASSERT("sub index out of bounds, returning NULL");
    result = NULL;
  }

  return result;
}

BT_INTERNAL bt_bool
bt_is_node_leaf(BT_Node *node)
{
  bt_u32 i;
  for (i = 0; i < BT_COUNTOF(node->subs); ++i) {
    if (node->subs[i] != 0) {
      return bt_false;
    }
  }
  return bt_true;
}

BT_INTERNAL void
bt_shift_keys_left(BT_Node *node, bt_u32 key_index)
{
  bt_u32 i;
  BT_ASSERT(node->key_count > 0);
  for (i = key_index; i < node->key_count - 1; ++i) {
    BT_Key *key;

    key = bt_node_get_key(node, i + 1);
    bt_node_set_key(node, i, key->id, key->data);
    bt_node_invalidate_key(node, i + 1);
  }
}

BT_INTERNAL void
bt_shift_keys_right(BT_Node *node, bt_u32 start_key)
{
  bt_s32 i;
  for (i = BT_COUNTOF(node->keys) - 2; i >= 0; --i) {
    BT_Key *key;

    if (i < (bt_u32)start_key) {
      break;
    }
    key = bt_node_get_key(node, i);
    bt_node_set_key(node, i + 1, key->id, key->data);
    bt_node_invalidate_key(node, i);
  }
}

BT_INTERNAL void
bt_shift_subs_left(BT_Node *node, bt_u32 key_index)
{
  bt_s32 i;
  for (i = (bt_s32)key_index; i < (bt_s32)node->key_count; ++i) {
    BT_Node *sub;
    sub = bt_node_get_sub(node, i + 1);
    bt_node_set_sub(node, i, sub);
    bt_node_set_sub(node, i + 1, NULL);
  }
  bt_node_set_sub(node, node->key_count, NULL);
}

BT_INTERNAL void
bt_shift_subs_right(BT_Node *node, bt_u32 key_index)
{
  bt_s32 i;
  for (i = (bt_s32)node->key_count - 1; i >= (bt_s32)key_index; --i) {
    BT_Node *sub;

    sub = bt_node_get_sub(node, i);
    bt_node_set_sub(node, i + 1, sub);
    bt_node_set_sub(node, i, NULL);
  }
}

BT_INTERNAL BT_ErrorCode
bt_push_stack_frame(BT_Context *tree, BT_Node *node, BT_KeyID key_id)
{
  BT_StackFrame *frame;

  if (tree->frames_max == 0) {
    tree->frames_count = 0;
    tree->frames_max = 32;
    tree->frames = (BT_StackFrame *)bt_malloc(sizeof(BT_StackFrame) * tree->frames_max, tree->malloc_ud);
    if (tree->frames == NULL) {
      return BT_ERROR_AllocationFailed;
    }
  }

  if (tree->frames_count >= tree->frames_max) {
    bt_u32 temp_max = tree->frames_max*2;
    BT_StackFrame *temp = (BT_StackFrame *)bt_malloc(temp_max*sizeof(BT_StackFrame), tree->malloc_ud);
    bt_memcpy(temp, tree->frames, tree->frames_count*sizeof(BT_StackFrame));
    bt_free(tree->frames, tree->malloc_ud);
    tree->frames_max = temp_max;
    tree->frames = temp;
    if (tree->frames == NULL) {
      return BT_ERROR_AllocationFailed;
    }
  }

  tree->frames[tree->frames_count].node = node;
  tree->frames[tree->frames_count].key_index = key_id;
  tree->frames_count += 1;

  return BT_ERROR_Ok;
}

BT_INTERNAL BT_ErrorCode
bt_pop_stack_frame(BT_Context *tree, BT_StackFrame *frame_out)
{
  BT_ErrorCode error_code = BT_ERROR_OpDenied;

  if (tree->frames_count > 0) {
    *frame_out = tree->frames[--tree->frames_count];
    error_code = BT_ERROR_Ok;
  }

  return error_code;
}

BT_INTERNAL BT_ErrorCode
bt_peek_stack_frame(BT_Context *tree, BT_StackFrame *frame_out)
{
  BT_ErrorCode error_code = BT_ERROR_OpDenied;

  if (tree->frames_count > 0) {
    *frame_out = tree->frames[tree->frames_count - 1];
    error_code = BT_ERROR_Ok;
  }

  return error_code;
}

BT_INTERNAL void
bt_reset_stack(BT_Context *tree)
{
  tree->frames_count = 0;
}

BT_API BT_ErrorCode
bt_create(BT_Context *tree, bt_u32 value_size, void *malloc_ud)
{
  tree->malloc_ud = malloc_ud;
  tree->value_size = value_size;
  tree->frames = NULL;
  tree->frames_count = 0;
  tree->frames_max = 0;
  tree->root = NULL;
  return BT_ERROR_Ok;
}

BT_API BT_ErrorCode
bt_destroy(BT_Context *tree)
{
  BT_Node *node = tree->root;

  while (node != NULL) {
    if (bt_is_node_leaf(node)) {
      BT_StackFrame frame;

      bt_free(node, tree->malloc_ud);
      node = NULL;
      while (bt_pop_stack_frame(tree, &frame) == BT_ERROR_Ok) {
        frame.key_index += 1;

        if (frame.key_index > frame.node->key_count) {
          /* NOTE(nick): Traversed all sub nodes and returned back to the parent node. */
          bt_free(frame.node, tree->malloc_ud);
        } else {
          BT_Node *sub = bt_node_get_sub(frame.node, frame.key_index);
          if (sub != NULL) {
            BT_ErrorCode error_code;

            error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
            if (error_code != BT_ERROR_Ok) {
              return error_code;
            }

            node = sub;
            BT_ASSERT(node->key_count > 0);

            break;
          }
        }
      }
    } else {
      BT_ErrorCode error_code;

      /* NOTE(nick): Descending down to a first sub-node */
      BT_ASSERT(node->key_count > 0);

      error_code = bt_push_stack_frame(tree, node, 0);
      if (error_code != BT_ERROR_Ok) {
        return error_code;
      }

      node = bt_node_get_sub(node, 0);
    }
  }

  bt_free(tree->frames, tree->malloc_ud);
  tree->frames_count = 0;
  tree->frames_max = 0;
  tree->frames = NULL;

  tree->root = NULL;

  return BT_ERROR_Ok;
}

BT_API BT_Key *
bt_search(BT_Context *tree, BT_KeyID id, bt_bool get_nearest)
{
  BT_Node *node = tree->root;
  BT_KeyID nearest_delta = BT_INVALID_ID;
  BT_Key *nearest_key = 0;
  while (node) {
    bt_s32 min, max, mid;

    BT_ASSERT(node->key_count > 0);
    max = node->key_count - 1;
    min = 0;
    mid = 0;

    while (min < max) {
      mid = (max - min + 1) / 2;
      BT_ASSERT(mid < node->key_count);
      if (id < node->keys[mid].id) {
        max = mid - 1;
      } else if (id > node->keys[mid].id) {
        min = mid + 1;
      } else {
        return &node->keys[mid];
      }
    }

    // check for nearest key
    BT_ASSERT(max < node->key_count);
    {
      bt_s64 delta = (bt_s64)id - (bt_s64)node->keys[max].id;
      if (delta >= 0 && delta <= nearest_delta) {
        nearest_key = &node->keys[max];
        nearest_delta = delta;
      }
    }

    if (id > node->keys[max].id) {
      node = node->subs[max + 1];
    } else if (id < node->keys[max].id) {
      node = node->subs[max];
    } else if (id == node->keys[max].id) {
      return &node->keys[max];
    }
  }

  if (!get_nearest) {
    nearest_key = NULL;
  }
  return nearest_key;
}

BT_API BT_ErrorCode
bt_insert(BT_Context *tree, BT_KeyID id, const void *data)
{
  BT_ErrorCode error_code = BT_ERROR_OpDenied;
  BT_StackFrame frame;

  if (tree->root == NULL) {
    tree->root = bt_new_node(tree);
    if (tree->root == NULL) {
      return BT_ERROR_AllocationFailed;
    }
  }

  {
    BT_Node *node = tree->root;
    bt_reset_stack(tree);
    while (node != NULL) {
      bt_u32 key_index;
      BT_ErrorCode error_code;

      for (key_index = 0; key_index < node->key_count; ++key_index) {
        BT_Key *key = bt_node_get_key(node, key_index);
        if (key->id == id) {
          return BT_ERROR_Ok;
        } else if (id < key->id) {
          break;
        }
      }
      /* NOTE(nick): Pushing frame in case we need to split. */
      error_code = bt_push_stack_frame(tree, node, key_index);
      if (error_code != BT_ERROR_Ok) {
        return error_code;
      }

      node = bt_node_get_sub(node, key_index);
    }
  }

  error_code = bt_peek_stack_frame(tree, &frame);
  if (error_code != BT_ERROR_Ok) {
    return error_code;
  }
  BT_ASSERT(frame.node->key_count < BT_COUNTOF(frame.node->keys));
  bt_shift_keys_right(frame.node, frame.key_index);
  frame.node->key_count += 1;
  bt_node_set_key(frame.node, frame.key_index, id, data);
  error_code = BT_ERROR_Ok;

  {
    while (bt_pop_stack_frame(tree, &frame) == BT_ERROR_Ok) {
      BT_Node *node_split = NULL;
      BT_Key median_key;
      bt_u32 i;

      error_code = BT_ERROR_RebalanceFailed;
      if (frame.node->key_count < BT_COUNTOF(frame.node->keys)) {
        break;
      }

      node_split = bt_new_node(tree);
      if (node_split == NULL) {
        error_code = BT_ERROR_AllocationFailed;
        break;
      }

      /* NOTE(nick): Copy upper-half of the sub-nodes to the split node. */
      for (i = BT_COUNTOF(frame.node->subs) / 2; i < BT_COUNTOF(frame.node->subs); ++i) {
        BT_Node *sub = bt_node_get_sub(frame.node, i);
        bt_node_set_sub(node_split, i - BT_COUNTOF(node_split->subs) / 2, sub);
        bt_node_set_sub(frame.node, i, NULL);
      }

      /* NOTE(nick): Copy upper-half of the keys to the split node. */
      for (i = BT_COUNTOF(frame.node->keys) / 2; i < BT_COUNTOF(frame.node->keys); ++i) {
        BT_Key *key = bt_node_get_key(frame.node, i);
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

      BT_ASSERT(node_split->key_count > 0);
      BT_ASSERT(frame.node->key_count > 0);

      { 
        /* NOTE(nick): Inserting a new median key. */

        BT_StackFrame frame_parent;

        if (bt_peek_stack_frame(tree, &frame_parent) == BT_ERROR_Ok) {
          BT_ASSERT(frame_parent.node->key_count < BT_COUNTOF(frame_parent.node->keys));
          bt_shift_keys_right(frame_parent.node, frame_parent.key_index);
          bt_node_set_key(frame_parent.node, frame_parent.key_index, median_key.id, median_key.data);

          frame_parent.node->key_count += 1;
          frame_parent.key_index += 1;

          bt_shift_subs_right(frame_parent.node, frame_parent.key_index);
          BT_ASSERT(bt_node_get_sub(frame_parent.node, frame_parent.key_index) == NULL);
          bt_node_set_sub(frame_parent.node, frame_parent.key_index, node_split);
        } else {
          /* NOTE(nick): Splitting reached root node, inserting a new root. */
          BT_Node *new_root = bt_new_node(tree);
          if (new_root != NULL) {
            bt_node_add_key(new_root, median_key.id, median_key.data);
            bt_node_set_sub(new_root, 0, frame.node);
            bt_node_set_sub(new_root, 1, node_split);
            tree->root = new_root;
          } else {
            error_code = BT_ERROR_AllocationFailed;
            break;
          }
        }
      }
    }
  }

  return error_code;
}

BT_API BT_ErrorCode
bt_delete(BT_Context *tree, BT_KeyID id)
{
  BT_Node *node = tree->root;
  BT_Node *node_delete = NULL;
  BT_KeyID key_index_delete = BT_INVALID_ID;

  bt_reset_stack(tree);
  while (node && node_delete == NULL) {
    bt_u32 key_index;
    BT_ErrorCode error_code;

    for (key_index = 0; key_index < node->key_count; ++key_index) {
      BT_Key *key = bt_node_get_key(node, key_index);
      if (key->id == id) {
        node_delete = node;
        key_index_delete = key_index;
        break;
      } else if (id < key->id) {
        break;
      }
    }

    error_code = bt_push_stack_frame(tree, node, key_index);
    if (error_code != BT_ERROR_Ok) {
      return error_code;
    }
    node = bt_node_get_sub(node, key_index);
  }

  if (node_delete == NULL) {
    return BT_ERROR_IDNotFound;
  }

  {
    /* NOTE(nick): Picking largest ID on the left branch of node_delete and inserting it
     * instead of the key that is being deleted, because after delete branch becomes
     * unbalanced and to compensate for it we insert largest ID. */

    BT_Node *node_new_separator = NULL;

    while (node != NULL) {
      BT_Key *key;
      BT_Key *key_separator;
      BT_ErrorCode error_code;

      error_code = bt_push_stack_frame(tree, node, node->key_count);
      if (error_code != BT_ERROR_Ok) {
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
      BT_Key *key;
      BT_Node *sub;

      BT_ASSERT(node_new_separator->key_count > 0);
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
    BT_StackFrame frame, frame_parent;
    BT_Node *node_right, *node_left;
    BT_Node *node_separator;
    BT_Node *sub;
    BT_Node *node_deficient;
    BT_Key *key;
    bt_u32 key_index_separator;

    if (bt_pop_stack_frame(tree, &frame) != BT_ERROR_Ok) {
      break;
    }
    if (bt_peek_stack_frame(tree, &frame_parent) != BT_ERROR_Ok) {
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
    if ((key_index_separator + 1) < BT_COUNTOF(node_separator->subs)) {
      node_right = node_separator->subs[key_index_separator + 1];
    }

    if (node_right != NULL && node_right->key_count > 1) {
      BT_ASSERT(node_deficient->subs[2] == NULL);
      BT_ASSERT(frame_parent.key_index == key_index_separator);

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
      BT_ASSERT(node_deficient->subs[2] == NULL);
      BT_ASSERT(frame_parent.key_index == key_index_separator);
      BT_ASSERT(key_index_separator > 0);

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
      BT_Node *node_dst;
      BT_Node *node_src;
      bt_s32 copy_count, i;

      if (node_left != NULL) {
        BT_ASSERT(node_left->key_count < BT_COUNTOF(node_left->keys));
        BT_ASSERT(key_index_separator > 0 );

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
        BT_ASSERT(node_dst->key_count == 0);

        node_dst->keys[node_dst->key_count++] = node_separator->keys[key_index_separator];
        BT_ASSERT(key_index_separator == frame_parent.key_index);
        bt_shift_keys_left(node_separator, key_index_separator);
      }

      copy_count = (bt_s32)node_src->key_count;
      if ((node_dst->key_count + copy_count) >= BT_COUNTOF(node_dst->keys)) {
        copy_count = BT_COUNTOF(node_dst->keys) - node_src->key_count - node_dst->key_count - 1;
      }

      for (i = 0; i < copy_count; ++i) {
        BT_ASSERT(node_dst->key_count < BT_COUNTOF(node_dst->keys));
        BT_ASSERT(node_src->key_count < BT_COUNTOF(node_src->keys));

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
          bt_free(tree->root, tree->malloc_ud);
          tree->root = node_dst;
        }

        if (node_src == node_right) {
          BT_ASSERT(bt_node_get_sub(node_separator, key_index_separator + 1) == node_src);
          bt_shift_subs_left(node_separator, key_index_separator + 1);
        }

        bt_free(node_src, tree->malloc_ud);
        node_src = NULL;
      }

      BT_ASSERT(node_separator->key_count > 0);
      bt_node_invalidate_key(node_separator, node_separator->key_count - 1);
      node_separator->key_count -= 1;
    }
  }

  return BT_ERROR_Ok;
}

BT_API BT_ErrorCode
bt_visit_keys(BT_Context *tree, BT_VisitNodesMode mode, void *user_context, bt_visit_keys_sig *visit)
{
  BT_Node *node;
  bt_u32 i;

  if (tree->root == NULL) {
    return BT_ERROR_Ok;
  } 

  if (visit == NULL) {
    return BT_ERROR_OpDenied;
  }

  node = tree->root;
  bt_reset_stack(tree);

  switch (mode) {
  case BT_VISIT_NODE_TopDown: {
    while (node != NULL) {
      for (i = 0; i < node->key_count; ++i) {
        BT_Key *key = bt_node_get_key(node, i);
        if (visit(user_context, key->id, key->data) == bt_false) {
          return BT_ERROR_Ok;
        }
      }

      if (bt_is_node_leaf(node)) {
        BT_StackFrame frame;

        node = NULL;
        while (bt_pop_stack_frame(tree, &frame) == BT_ERROR_Ok) {
          frame.key_index += 1;
          if (frame.key_index <= frame.node->key_count) {
            BT_Node *sub = bt_node_get_sub(frame.node, frame.key_index);
            if (sub != NULL) {
              BT_ErrorCode error_code;

              error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
              if (error_code != BT_ERROR_Ok) {
                return error_code;
              }

              node = sub;
              BT_ASSERT(node->key_count > 0);
              break;
            }
          }
        }
      } else {
        BT_ErrorCode error_code;

        BT_ASSERT(node->key_count > 0);
        error_code = bt_push_stack_frame(tree, node, 0);
        if (error_code != BT_ERROR_Ok) {
          return error_code;
        }

        node = bt_node_get_sub(node, 0);
      }
    }
  } break;

  case BT_VISIT_NODE_BottomUp: {
    while (node != NULL) {
      if (bt_is_node_leaf(node)) {
        BT_StackFrame frame;

        for (i = 0; i < node->key_count; ++i) {
          BT_Key *key = bt_node_get_key(node, i);
          if (visit(user_context, key->id, key->data) == bt_false) {
            return BT_ERROR_Ok;
          }
        }

        node = NULL;
        while (bt_pop_stack_frame(tree, &frame) == BT_ERROR_Ok) {
          frame.key_index += 1;

          if (frame.key_index > frame.node->key_count) {
            for (i = 0; i < frame.node->key_count; ++i) {
              BT_Key *key = bt_node_get_key(frame.node, i);
              if (visit(user_context, key->id, key->data) == bt_false) {
                return BT_ERROR_Ok;
              }
            }
          } else {
            BT_Node *sub = bt_node_get_sub(frame.node, frame.key_index);
            if (sub != NULL) {
              BT_ErrorCode error_code;

              error_code = bt_push_stack_frame(tree, frame.node, frame.key_index);
              if (error_code != BT_ERROR_Ok) {
                return error_code;
              }

              node = sub;
              BT_ASSERT(node->key_count > 0);
              break;
            }
          }
        }
      } else {
        BT_ErrorCode error_code;

        BT_ASSERT(node->key_count > 0);
        error_code = bt_push_stack_frame(tree, node, 0);
        if (error_code != BT_ERROR_Ok) {
          return error_code;
        }

        node = bt_node_get_sub(node, 0);
      }
    }
  } break;

  default: break;
  }

  return BT_ERROR_Ok;
}

#if 0
BT_INTERNAL void
bt_dump_stack(BT_Context *tree)
{
  bt_u32 i;

  bt_debug_printf("Dumping node stack:\n");
  for (i = 0; i < tree->frames_count; ++i) {
    BT_StackFrame *frame = &tree->frames[i];
    if (frame->key_index < BT_COUNTOF(frame->node->keys)) {
      bt_debug_printf("%d: key_index: %d, id: %d\n", i, frame->key_index, frame->node->keys[frame->key_index].id);
    } else {
      bt_debug_printf("%d: key_index: %d, no id\n", i, frame->key_index);
    }
  }
}

BT_INTERNAL void
bt_dump_node_keys(BT_Node *node)
{
  bt_u32 i;
  for (i = 0; i < node->key_count; ++i) {
    bt_debug_printf("%d ", node->keys[i].id);
  }
  bt_debug_printf("\n");

  for (i = 0; i < BT_COUNTOF(node->subs); ++i) {
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

BT_INTERNAL void
bt_dump_tree(BT_Context *tree, x_arena *arena)
{
  BT_Node *node = tree->root;
  bt_u32 depth = 0;
  x_arena_temp temp;
  BT_StackFrame *stack = 0;

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
      BT_StackFrame *frame = x_arena_push_struct(arena, BT_StackFrame);
      frame->next = stack;
      stack = frame;
      frame->node = node;
      frame->key_index = 0;
      node = node->subs[0];
    }
  }

  x_arena_temp_end(temp);
}
#endif

#endif /* BT_IMPLEMENTATION */
