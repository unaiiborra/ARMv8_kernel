#pragma once

#include <stdint.h>

// Red black tree, based on the Linux implementation


typedef struct {
    struct rbtnode* root;
} rbtree_t;

typedef struct {
    uint64_t _[3];
    /* union {
    int64_t  key: signed
    uint64_t key: unsigned
    uint8_t  data[]: use manual conditions
     }
    */
} rb_header_t;

typedef enum {
    RBT_LESS_THAN,
    RBT_GREATER_THAN,
    RBT_EQUALS,
} rbt_condition_e;

typedef rbt_condition_e (*rbt_condition_t)(void* node_a, void* node_b);

void* rbt_find_i64(rbtree_t* tree, int64_t key);
void* rbt_find_u64(rbtree_t* tree, uint64_t key);
void* rbt_find(rbtree_t* tree, rbt_condition_t cond, void* node);

void rbt_insert_i64(rbtree_t* tree, void* node);
void rbt_insert_u64(rbtree_t* tree, void* node);
void rbt_insert(rbtree_t* tree, void* node, rbt_condition_t cond);

void* rbt_remove(rbtree_t* tree, void* node);
