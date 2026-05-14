#include <kernel/lib/rbtree.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdnoreturn.h>

#define always_inline __attribute((always_inline)) static inline

typedef enum {
    BLACK,
    RED,
} color_e;

typedef struct rbtnode {
    struct rbtnode *left, *right;
    uint64_t        packed;
    union {
        uint64_t key_unsigned;
        int64_t  key_signed;
        uint8_t  T;
    };
} rbtnode_t;

_Static_assert(sizeof(rb_header_t) == 24, "rb_header_t size mismatch");



always_inline color_e get_color(rbtnode_t* node)
{
    return node->packed & 1;
}

always_inline rbtnode_t* get_parent(rbtnode_t* node)
{
    return (rbtnode_t*)(node->packed & ~UINT64_C(1));
}

always_inline bool is_black(rbtnode_t* node)
{
    return (node->packed & 1) == 0;
}

always_inline bool is_red(rbtnode_t* node)
{
    return (node->packed & 1) == 1;
}

void set_color(rbtnode_t* node, color_e color)
{
    (color == 0) ? (node->packed &= ~UINT64_C(1))
                 : (node->packed |= UINT64_C(1));
}

always_inline void set_parent(rbtnode_t* node, rbtnode_t* parent)
{
    node->packed &= UINT64_C(1);
    node->packed |= (((uint64_t)parent) & ~UINT64_C(1));
}

always_inline void set_parent_color(
    rbtnode_t* node,
    rbtnode_t* parent,
    color_e    color)
{
    node->packed = ((uint64_t)parent & ~UINT64_C(1)) | (uint64_t)color;
}

always_inline void rotate_set_parents(
    rbtnode_t* old,
    rbtnode_t* new,
    rbtree_t*  root,
    color_e    color)
{
    rbtnode_t* parent = get_parent(old);

    set_parent(new, get_parent(old));
    set_color(new, get_color(old));

    set_parent_color(old, new, color);

    if (parent) {
        if (parent->left == old)
            parent->left = new;
        else
            parent->right = new;
    }
    else {
        root->root = new;
    }
}

void* rbt_find_i64(rbtree_t* tree, int64_t key)
{
    rbtnode_t* node = tree->root;

    while (node && node->key_signed != key)
        node = key < node->key_signed ? node->left : node->right;

    return node;
}

void* rbt_find_u64(rbtree_t* tree, uint64_t key)
{
    rbtnode_t* node = tree->root;

    while (node && node->key_unsigned != key)
        node = key < node->key_unsigned ? node->left : node->right;

    return node;
}

void* rbt_find(rbtree_t* tree, rbt_condition_t cond, void* node_a)
{
    rbtnode_t* node_b = tree->root;

    while (node_b) {
        rbt_condition_e cmp = cond(node_a, node_b);

        if (cmp == RBT_EQUALS)
            return node_b;

        node_b = (cmp == RBT_LESS_THAN) ? node_b->left : node_b->right;
    }

    return node_b;
}

always_inline bool bst_insert_unsigned(rbtree_t* tree, rbtnode_t* node)
{
    rbtnode_t* parent = NULL;
    rbtnode_t* cur    = tree->root;

    while (cur) {
        parent = cur;

        if (node->key_unsigned == cur->key_unsigned)
            PANIC("key already inserted");

        if (node->key_unsigned < cur->key_unsigned)
            cur = cur->left;
        else
            cur = cur->right;
    }

    node->left  = NULL;
    node->right = NULL;
    set_parent_color(node, parent, RED);

    if (!parent) {
        tree->root = node;
        set_color(node, BLACK);

        return false;
    }

    if (node->key_unsigned < parent->key_unsigned)
        parent->left = node;
    else
        parent->right = node;

    return true;
}

always_inline bool bst_insert_signed(rbtree_t* tree, rbtnode_t* node)
{
    rbtnode_t* parent = NULL;
    rbtnode_t* cur    = tree->root;

    while (cur) {
        parent = cur;

        if (node->key_signed == cur->key_signed)
            PANIC("key already inserted");

        if (node->key_signed < cur->key_signed)
            cur = cur->left;
        else
            cur = cur->right;
    }

    node->left  = NULL;
    node->right = NULL;
    set_parent_color(node, parent, RED);

    if (!parent) {
        tree->root = node;
        set_color(node, BLACK);

        return false;
    }

    if (node->key_signed < parent->key_signed)
        parent->left = node;
    else
        parent->right = node;

    return true;
}

always_inline bool bst_insert_conditional(
    rbtree_t*       tree,
    rbtnode_t*      node,
    rbt_condition_t cond)
{
    rbtnode_t* parent = NULL;
    rbtnode_t* cur    = tree->root;

    while (cur) {
        parent = cur;

        rbt_condition_e cmp = cond(node, cur);

        if (cmp == RBT_EQUALS)
            PANIC("key already inserted");

        if (cmp == RBT_LESS_THAN)
            cur = cur->left;
        else
            cur = cur->right;
    }

    node->left  = NULL;
    node->right = NULL;
    set_parent_color(node, parent, RED);

    if (!parent) {
        tree->root = node;
        set_color(node, BLACK);

        return false;
    }

    if (cond(node, parent) == RBT_LESS_THAN)
        parent->left = node;
    else
        parent->right = node;

    return true;
}

static void insert_rebalance(rbtree_t* tree, rbtnode_t* node)
{
    rbtnode_t *parent, *gparent, *tmp;

    parent = get_parent(node);

    while (true) {
        if (!parent) {
            set_parent_color(node, NULL, BLACK);
            break;
        }

        if (is_black(parent))
            break;

        gparent = get_parent(parent);

        tmp = gparent->right;
        if (parent != tmp) {
            if (tmp && is_red(tmp)) {
                set_parent_color(tmp, gparent, BLACK);
                set_parent_color(parent, gparent, BLACK);
                node   = gparent;
                parent = get_parent(node);
                set_parent_color(node, parent, RED);
                continue;
            }

            tmp = parent->right;
            if (node == tmp) {
                tmp           = node->left;
                parent->right = tmp;
                node->left    = parent;

                if (tmp)
                    set_parent_color(tmp, parent, BLACK);

                set_parent_color(parent, node, RED);

                parent = node;
                tmp    = node->right;
            }

            gparent->left = tmp;
            parent->right = gparent;

            if (tmp) {
                set_parent_color(tmp, gparent, BLACK);
            }

            rotate_set_parents(gparent, parent, tree, RED);
            break;
        }
        else {
            tmp = gparent->left;

            if (tmp && is_red(tmp)) {
                set_parent_color(tmp, gparent, BLACK);
                set_parent_color(parent, gparent, BLACK);
                node   = gparent;
                parent = get_parent(node);
                set_parent_color(node, parent, RED);
                continue;
            }

            tmp = parent->left;
            if (node == tmp) {
                tmp          = node->right;
                parent->left = tmp;
                node->right  = parent;

                if (tmp)
                    set_parent_color(tmp, parent, BLACK);

                set_parent_color(parent, node, RED);
                parent = node;
                tmp    = node->left;
            }

            gparent->right = tmp;
            parent->left   = gparent;

            if (tmp)
                set_parent_color(tmp, gparent, BLACK);

            rotate_set_parents(gparent, parent, tree, RED);
            break;
        }
    }
}

static void erase_color(rbtree_t* tree, rbtnode_t* parent)
{
    rbtnode_t *node = NULL, *sibling, *tmp1, *tmp2;

    while (true) {
        sibling = parent->right;
        if (node != sibling) {
            if (is_red(sibling)) {
                tmp1          = sibling->left;
                parent->right = tmp1;
                sibling->left = parent;
                set_parent_color(tmp1, parent, BLACK);
                rotate_set_parents(parent, sibling, tree, RED);
                sibling = tmp1;
            }
            tmp1 = sibling->right;
            if (!tmp1 || is_black(tmp1)) {
                tmp2 = sibling->left;
                if (!tmp2 || is_black(tmp2)) {
                    set_parent_color(sibling, parent, RED);
                    if (is_red(parent))
                        set_color(parent, BLACK);
                    else {
                        node   = parent;
                        parent = get_parent(node);

                        if (parent)
                            continue;
                    }
                    break;
                }

                tmp1          = tmp2->right;
                sibling->left = tmp1;
                tmp2->right   = sibling;
                parent->right = tmp2;
                if (tmp1)
                    set_parent_color(tmp1, sibling, BLACK);
                tmp1    = sibling;
                sibling = tmp2;
            }

            tmp2          = sibling->left;
            parent->right = tmp2;
            sibling->left = parent;
            set_parent_color(tmp1, sibling, BLACK);
            if (tmp2)
                set_parent(tmp2, parent);

            rotate_set_parents(parent, sibling, tree, BLACK);
            break;
        }
        else {
            sibling = parent->left;
            if (is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                tmp1           = sibling->right;
                parent->left   = tmp1;
                sibling->right = parent;
                set_parent_color(tmp1, parent, BLACK);
                rotate_set_parents(parent, sibling, tree, RED);
                sibling = tmp1;
            }
            tmp1 = sibling->left;
            if (!tmp1 || is_black(tmp1)) {
                tmp2 = sibling->right;
                if (!tmp2 || is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    set_parent_color(sibling, parent, RED);

                    if (is_red(parent))
                        set_color(parent, BLACK);

                    else {
                        node   = parent;
                        parent = get_parent(node);

                        if (parent)
                            continue;
                    }
                    break;
                }
                tmp1           = tmp2->left;
                sibling->right = tmp1;
                tmp2->left     = sibling;
                parent->left   = tmp2;
                if (tmp1)
                    set_parent_color(tmp1, sibling, BLACK);
                tmp1    = sibling;
                sibling = tmp2;
            }

            tmp2           = sibling->right;
            parent->left   = tmp2;
            sibling->right = parent;
            set_parent_color(tmp1, sibling, BLACK);

            if (tmp2)
                set_parent(tmp2, parent);

            rotate_set_parents(parent, sibling, tree, BLACK);
            break;
        }
    }
}

void rbt_insert_i64(rbtree_t* tree, void* node)
{
    ASSERT(tree && node, "invalid params!");

    bool rebalance = bst_insert_signed(tree, node);

    if (rebalance)
        insert_rebalance(tree, node);
}

void rbt_insert_u64(rbtree_t* tree, void* node)
{
    ASSERT(tree && node, "invalid params!");

    bool rebalance = bst_insert_unsigned(tree, node);

    if (rebalance)
        insert_rebalance(tree, node);
}

void rbt_insert(rbtree_t* tree, void* node, rbt_condition_t cond)
{
    ASSERT(tree && node && cond, "invalid params!");

    bool rebalance = bst_insert_conditional(tree, node, cond);

    if (rebalance)
        insert_rebalance(tree, node);
}

void* rbt_remove(rbtree_t* tree, void* n)
{
    rbtnode_t* node = n;

    rbtnode_t* child = node->right;
    rbtnode_t* tmp   = node->left;
    rbtnode_t *parent, *rebalance;

    if (!tmp) {
        parent = get_parent(node);

        if (parent) {
            if (parent->left == node)
                parent->left = child;
            else
                parent->right = child;
        }
        else {
            tree->root = child;
        }

        if (child) {
            set_parent_color(child, parent, get_color(node));

            rebalance = NULL;
        }
        else {
            rebalance = (is_black(node)) ? parent : NULL;
        }
    }
    else if (!child) {
        parent = get_parent(node);
        set_parent_color(tmp, parent, get_color(node));

        if (parent) {
            if (parent->left == node)
                parent->left = tmp;
            else
                parent->right = tmp;
        }
        else {
            tree->root = tmp;
        }

        rebalance = NULL;
    }
    else {
        rbtnode_t* successor = child;
        rbtnode_t* child2;

        tmp = child->left;
        if (!tmp) {
            parent = successor;
            child2 = successor->right;
        }
        else {
            do {
                parent    = successor;
                successor = tmp;
                tmp       = tmp->left;
            } while (tmp);

            child2           = successor->right;
            parent->left     = child2;
            successor->right = child;
            set_parent(child, successor);
        }

        successor->left = node->left;
        set_parent(node->left, successor);

        rbtnode_t* node_parent = get_parent(node);
        if (node_parent) {
            if (node_parent->left == node)
                node_parent->left = successor;
            else
                node_parent->right = successor;
        }
        else {
            tree->root = successor;
        }

        if (child2) {
            set_parent_color(child2, parent, BLACK);
            rebalance = NULL;
        }
        else {
            rebalance = (is_black(successor)) ? parent : NULL;
        }

        set_parent_color(successor, node_parent, get_color(node));
    }

    if (rebalance)
        erase_color(tree, rebalance);

    return node;
}