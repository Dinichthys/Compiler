#include "simplify_diff.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "struct_lang.h"
#include "connect_tree_lang.h"

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"

static node_t* SimplifyNum     (node_t* const node, const double number);
static node_t* SimplifyAddZero (node_t* const node);
static node_t* SimplifyMulOne  (node_t* const node);
static node_t* SimplifyFuncs   (node_t* const root);
static bool IdentityNodeValNum (const node_t* const node, const double number);

node_t* Simplify (node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    LOG (kDebug, "Root at start = %p\n", root);

    node_t* new_root = root;

    const double zero = 0;
    const double one  = 1;

    if ((root->right == NULL) && (root->left == NULL) || (root->type != kArithm))
    {
        return root;
    }

    bool left_is_zero_num  = IdentityNodeValNum (new_root->left,  zero);
    bool right_is_zero_num = IdentityNodeValNum (new_root->right, zero);

    if (   ((new_root->value.operation == kAdd) && (left_is_zero_num || right_is_zero_num))
        || ((new_root->value.operation == kSub) && (right_is_zero_num)))
    {
        new_root = SimplifyAddZero (new_root);

        if (new_root->type != kFunc)
        {
            return new_root;
        }
        left_is_zero_num  = IdentityNodeValNum (new_root->left,  zero);
        right_is_zero_num = IdentityNodeValNum (new_root->right, zero);
        ConnectTree (new_root);
    }

    LOG (kDebug, "New root after simplification (adding null) = %p\n", new_root);

    if ((new_root->value.operation == kMul)
        && ((IdentityNodeValNum (new_root->left,  one))
        ||  (IdentityNodeValNum (new_root->right, one))))
    {
        new_root = SimplifyMulOne (new_root);

        if (new_root->type != kFunc)
        {
            return new_root;
        }
        left_is_zero_num  = IdentityNodeValNum (new_root->left,  zero);
        right_is_zero_num = IdentityNodeValNum (new_root->right, zero);
        ConnectTree (new_root);
    }

    LOG (kDebug, "New root  after simplification (multiply by one) = %p\n", new_root);

    if (   ((new_root->value.operation == kMul) && (right_is_zero_num || left_is_zero_num))
        || ((new_root->value.operation == kDiv) && left_is_zero_num))
    {
        new_root = SimplifyNum (new_root, 0);

        if (new_root->type != kFunc)
        {
            return new_root;
        }
        left_is_zero_num  = IdentityNodeValNum (new_root->left,  zero);
        right_is_zero_num = IdentityNodeValNum (new_root->right, zero);
        ConnectTree (new_root);
    }

    LOG (kDebug, "New root after simplification (multiply null or divide zero) = %p\n", new_root);

    if ((  (new_root->left  != NULL) && (new_root->left->type  == kNum)
        && (new_root->right != NULL) && (new_root->right->type == kNum))
        || ((new_root->left == NULL) && (new_root->right != NULL) && (new_root->right->type == kNum)))
    {
        new_root = SimplifyFuncs (new_root);

        if (new_root->type != kFunc)
        {
            return new_root;
        }
        left_is_zero_num  = IdentityNodeValNum (new_root->left,  zero);
        right_is_zero_num = IdentityNodeValNum (new_root->right, zero);
        ConnectTree (new_root);
    }

    LOG (kDebug, "New root after simplification (functions with constants) = %p\n", new_root);

    node_t* old_left  = new_root->left;
    node_t* old_right = new_root->right;

    if (new_root->left != NULL)
    {
        new_root->left = Simplify (new_root->left);
    }

    ConnectTree (new_root);

    if (new_root->right != NULL)
    {
        new_root->right = Simplify (new_root->right);
    }

    ConnectTree (new_root);

    if ((old_left == new_root->left) && (old_right == new_root->right))
    {
        return new_root;
    }

    return Simplify (new_root);
}

static node_t* SimplifyNum (node_t* const node, const double number)
{
    ASSERT (node != NULL, "Invalid argument root = %p\n", node);

    node_t* const parent = node->parent;

    TreeDtor (node);
    return AddNode ({.type = kArithm, {.number = number}, parent, NULL, NULL});
}

static node_t* SimplifyAddZero (node_t* const node)
{
    ASSERT (node != NULL, "Invalid argument root = %p\n", node);

    const double zero = 0;

    node_t* result = NULL;

    if (IdentityNodeValNum (node->left, zero))
    {
        TreeDtor (node->left);
        result = node->right;
        free (node);
        result->parent = NULL;
        return result;
    }

    LOG (kDebug, "Left node is not a node with null\n");

    if (IdentityNodeValNum (node->right, zero))
    {
        TreeDtor (node->right);
        result = node->left;
        free (node);
        result->parent = NULL;
        return result;
    }

    LOG (kDebug, "Right node is not a node with null\n");

    return result;
}

static node_t* SimplifyMulOne (node_t* const node)
{
    ASSERT (node != NULL, "Invalid argument root = %p\n", node);

    const double one = 1;

    if ((node->value.operation == kMul)
        && (IdentityNodeValNum (node->right, one)))
    {
        node_t* result = node->left;
        free (node->right);
        node->left->parent = NULL;
        free (node);
        return result;
    }

    if ((node->value.operation == kMul)
        && (IdentityNodeValNum (node->left, one)))
    {
        node_t* result = node->right;
        free (node->left);
        node->right->parent = NULL;
        free (node);
        return result;
    }

    return NULL;
}


static node_t* SimplifyFuncs (node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    LOG (kDebug, "Root = %p\n", root);

    #define BINARY_FUNC(func, op)                                                             \
        if (root->value.operation == func)                                                    \
        {                                                                                     \
            return SimplifyNum (root, root->left->value.number op root->right->value.number); \
        }

    BINARY_FUNC (kAdd, +);
    BINARY_FUNC (kSub, -);
    BINARY_FUNC (kMul, *);
    BINARY_FUNC (kDiv, /);

    #undef BINARY_FUNC

    if (root->value.operation == kPow)
    {
        return SimplifyNum (root, pow (root->left->value.number, root->right->value.number));
    }

    #define UNARY_FUNC(func, op)                                           \
        if (root->value.operation == func)                                 \
        {                                                                  \
            return SimplifyNum (root, op (root->right->value.number));     \
        }

    UNARY_FUNC (kSin, sin);
    UNARY_FUNC (kCos, cos);
    UNARY_FUNC (kTg,  tan);
    UNARY_FUNC (kCtg, ((double) 1) / tan);

    UNARY_FUNC (kLn, log);

    if (root->value.operation == kLog)
    {
        return SimplifyNum (root, log (root->right->value.number) / log (root->left->value.number));
    }

    #undef UNARY_FUNC

    return NULL;
}

static bool IdentityNodeValNum (const node_t* const node, const double number)
{
    return ((node != NULL) && (node->type == kNum) && (abs (node->value.number - number) < kAccuracy));
}
