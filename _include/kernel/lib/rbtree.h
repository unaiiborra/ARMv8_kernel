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

typedef enum : int32_t {
    RBT_LESS_THAN,
    RBT_GREATER_THAN,
    RBT_EQUALS,
    // it can define other codes that will be returned by rbt_insert
    // and match with the return value of rbt_condition_t. Custom codes do not
    // insert the new node but can be used to know why the node was not inserted.
    // EQUALS does not replace the existing node, returns RBT_FIND_EXISTS.
} rbt_condition_e;

typedef enum rbt_find_result_e : int32_t {
    RBT_FIND_OK       = 0,
    RBT_FIND_RESERVED = 1, // this result is reserved for internal use, it will
                           // never be returned by rbt_insert
    RBT_FIND_EXISTS = RBT_EQUALS,
    // other codes can be defined by the caller and match with the return value
    // of rbt_condition_t
} rbt_insert_result_e;

typedef rbt_condition_e (*rbt_condition_t)(void* node_a, void* node_b);

void* rbt_find_i64(rbtree_t* tree, int64_t key);
void* rbt_find_u64(rbtree_t* tree, uint64_t key);
void* rbt_find(rbtree_t* tree, rbt_condition_t cond, void* node);

rbt_insert_result_e rbt_insert_i64(rbtree_t* tree, void* node);
rbt_insert_result_e rbt_insert_u64(rbtree_t* tree, void* node);
rbt_insert_result_e rbt_insert(rbtree_t* tree, void* node, rbt_condition_t cond);

void* rbt_remove(rbtree_t* tree, void* node);
