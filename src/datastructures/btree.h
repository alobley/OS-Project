#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>
#include <stddef.h>

#define MAX_KEYS 255
#define MAX_CHILDREN 256

typedef struct B_Tree_Node {
    uint32_t n;
    uint32_t keys[MAX_KEYS];
    struct B_Tree_Node *children[MAX_CHILDREN];
    bool leaf;
} b_node_t;

#endif // BTREE_H