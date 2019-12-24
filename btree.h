#ifndef BTREE_HEADER_INCLUDE
#define BTREE_HEADER_INCLUDE

#define BTREE_ORDER 2
#define BTREE_NODE_COUNT (BTREE_ORDER * 2)
#define BTREE_KEY_COUNT (BTREE_NODE_COUNT - 1)

#define U32 unsigned int

#define BTREE_INVALID_ID UINT32_MAX
typedef int BTreeKeyID;


typedef struct BTreeKey {
    BTreeKeyID id;
    void *data;
    U32 data_size;
} BTreeKey;

typedef struct BTreeNode {
    U32 next_key_index;
    BTreeKey keys[BTREE_KEY_COUNT];
    struct BTreeNode *subs[BTREE_NODE_COUNT];
} BTreeNode;

typedef struct BTree {
    BTreeNode *root;
} BTree;

typedef struct BTreeStackFrame {
    BTreeNode *node;
    U32 key_index;
    struct BTreeStackFrame *next;
} BTreeStackFrame;

#endif /* BTREE_HEADER_INCLUDE */
