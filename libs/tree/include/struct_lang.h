#ifndef STRUCT_LANG_H
#define STRUCT_LANG_H

#include "language.h"

#define ADDNODE_FUNC(func_type, left_node, right_node)\
    AddNode ({.type = kFunc, {.operation = func_type}, .parent = NULL, .left = left_node, .right = right_node})

#define ADDNODE_SYM(symbol_type)\
    AddNode ({.type = kSym, {.operation = symbol_type}, .parent = NULL, .left = NULL, .right = NULL})

#define ADDNODE_NUM(value)\
    AddNode ({.type = kNum, {.number = value}, .parent = NULL, .left = NULL, .right = NULL})

#define PUT_ROOT_TO_THE_LEFT_SUBTREE                                                         \
    if (root->type != kNewNode)                                                              \
    {                                                                                        \
        root = AddNode ({.type = kNewNode, .parent = NULL, .left = root, .right = NULL});    \
        root->left->parent = root;                                                           \
    }

#define CREATE_ROOT_WITH_NODE_IN_LEFT_SUBTREE_AND_MAKE_NODE_POINT_ON_ROOT   \
    node_t* root = TreeCtor ();                                             \
    root->left = *node;                                                     \
    (*node)->parent = root;                                                 \
    *node = root;

node_t*        TreeCtor (void);
enum LangError TreeDtor (node_t* const root);
node_t*        AddNode        (const node_t set_val);
const char*    EnumErrorToStr (const enum LangError error);

#endif // STRUCT_LANG_H
