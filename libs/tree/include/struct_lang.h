#if !(defined(STRUCT_LANG_H))
#define STRUCT_LANG_H

#include "language.h"

#define ADDNODE_FUNC(func_type, left_node, right_node)\
    AddNode ({.type = kFunc, {.operation = func_type}, .parent = NULL, .left = left_node, .right = right_node})

#define ADDNODE_SYM(symbol_type)\
    AddNode ({.type = kSym, {.operation = symbol_type}, .parent = NULL, .left = NULL, .right = NULL})

#define ADDNODE_NUM(value)\
    AddNode ({.type = kNum, {.number = value}, .parent = NULL, .left = NULL, .right = NULL})

#define PUT_ROOT_TO_THE_LEFT_SUBTREE                                                                                    \
    if (root->type != kNewNode)                                                                                         \
    {                                                                                                                   \
        root = AddNode ({.type = kNewNode, {.operation = kUndefinedNode}, .parent = NULL, .left = root, .right = NULL});\
        root->left->parent = root;                                                                                      \
    }

#define CREATE_ROOT_WITH_NODE_IN_LEFT_SUBTREE_AND_MAKE_NODE_POINT_ON_ROOT   \
    node_t* root = TreeCtor ();                                             \
    root->left = *node;                                                     \
    (*node)->parent = root;                                                 \
    *node = root;

enum NodeType
{
    kNewNode = 0,

    kMainFunc = 1,

    kNum    = 2,
    kVar    = 3,
    kFunc   = 4,
    kArithm = 5,
    kCycle  = 6,
    kCond   = 7,
    kSym    = 8,
    kType   = 9,

    kUserFunc = 10,

    kEndToken = 11,

    kComp = 12,

    kRet = 13,

    kInvalidNodeType = -1,
};

typedef struct func_node
{
    char   func_name [kWordLen];
    size_t func_num;
    size_t cnt_args;
} func_node_t;

typedef struct var_node
{
    char   variable [kWordLen];
    long long index;
} var_node_t;

typedef struct node
{
    enum NodeType type;

    union value
    {
        double number;
        var_node_t    variable;
        func_node_t   function;
        enum OpType operation;
    } value;

    struct node* parent;

    struct node* left;
    struct node* right;
} node_t;

node_t*        TreeCtor (void);
enum LangError TreeDtor (node_t* const root);
node_t*        AddNode        (const node_t set_val);
const char*    EnumErrorToStr (const enum LangError error);

#endif // STRUCT_LANG_H
