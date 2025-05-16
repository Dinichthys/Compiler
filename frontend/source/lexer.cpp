#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "read_lang.h"
#include "struct_lang.h"
#include "connect_tree_lang.h"

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "list.h"

//-----DSL----------------------------------------------------------------------------------------------------

#define TOKEN_POSITION tokens [*token_index].number_of_line, tokens[*token_index].line_pos

#define SHIFT_TOKEN (*token_index)++;

#define REMOVE_TOKEN (*token_index)--;

#define CHECK_RESULT(...)           \
        if (result != kDoneLang)    \
        {                           \
            __VA_ARGS__             \
            return result;          \
        }

#define CHECK_TOKEN_OP(token_type, op_type)\
    ((tokens [*token_index].type == token_type) && (tokens [*token_index].value.operation == op_type))

#define TOKEN_TYPE tokens [*token_index].type

#define CHECK_END                                       \
        if (tokens [*token_index].type == kEndToken)    \
        {                                               \
            return kDoneLang;                           \
        }

#define TOKEN_OP_PATTERN {.type = TOKEN_TYPE, {.operation = tokens [*token_index].value.operation}, .parent = NULL, .left = NULL, .right = NULL}

#define CHECK_NULL_PTR(root)                \
    if (root == NULL)                       \
    {                                       \
        return SyntaxError (TOKEN_POSITION);\
    }

//------------------------------------------------------------------------------------------------------------

static const char* sFileName = NULL;

static enum LangError GetFunctionPattern (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                          list_t* const list, funcs_t* const funcs);
static enum LangError GetArgs         (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       variables_t* const variables);
static enum LangError GetCommand      (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables, node_t* const func);
static enum LangError GetFunctionCall (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetAssign       (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetCycle        (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables, node_t* const func);
static enum LangError GetIf           (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables, node_t* const func);
static enum LangError GetCond         (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetAddSub       (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetMulDiv       (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetPow          (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetBrace        (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetNumFunc      (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);

static enum LangError SyntaxError (const size_t number_nl, const size_t line_pos);

static long long FindVarInTable  (const char* const var, list_t list, variables_t variables);
static size_t    FindFuncInTable (const func_node_t function, const funcs_t* const funcs);

static enum LangError ConnectFuncCall (node_t* const  node, const funcs_t* const funcs);

enum LangError GetProgram (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                  const char* const file_name)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n", tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",   node);

    sFileName = file_name;

    enum LangError result = kDoneLang;

    list_t list = {};

    variables_t variables = {.var_num = 0, .var_table = {}};
    funcs_t funcs = {.func_num = 0, .func_table = {}};

    ListError result_list = ListCtor (&list, kVarTablesMaxNum, sizeof (variables));
    if (result_list != kDoneList)
    {
        return kCantCreateListOfTables;
    }

    if (*node == NULL)
    {
        *node = TreeCtor ();
    }
    node_t** root = node;

    while (TOKEN_TYPE != kEndToken)
    {
        if (TOKEN_TYPE != kType)
        {
            SyntaxError (TOKEN_POSITION);
            ListDtor (&list);
            return kMissTypeInGlobal;
        }

        if ((*root)->type == kType)
        {
            root = &((*root)->left);
        }

        if (*root == NULL)
        {
            *root = AddNode (TOKEN_OP_PATTERN);
            CHECK_NULL_PTR (*root);
        }
        else
        {
            **root = TOKEN_OP_PATTERN;
        }
        SHIFT_TOKEN;

        SHIFT_TOKEN;

        if (CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen))
        {
            REMOVE_TOKEN;
            result_list = ListPushFront (&list, &variables);
            result = GetFunctionPattern (tokens, token_index, &((*root)->right), &list, &funcs);
            if (result != kDoneLang)
            {
                ListDtor (&list);
                return result;
            }
            ListPopFront (&list, &variables);
            continue;
        }

        REMOVE_TOKEN;
        if ((variables.var_num >= kVarMaxNum)
            || (FindVarInTable (tokens [*token_index].value.variable, list, variables) != LLONG_MAX))
        {
            SyntaxError (tokens [*token_index].number_of_line, tokens [*token_index].line_pos);
            ListDtor (&list);
            return kTooManyVar;
        }
        strcpy (variables.var_table [variables.var_num++], tokens [*token_index].value.variable);
        result = GetAssign (tokens, token_index, &((*root)->right), &list, &variables);
        if (result != kDoneLang)
        {
            ListDtor (&list);
            return result;
        }
    }

    ListDtor (&list);

    return ConnectFuncCall (*node, &funcs);
}

static enum LangError GetFunctionPattern (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                          list_t* const list, funcs_t* const funcs)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n", tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",   node);
    ASSERT (list        != NULL, "Invalid argument list = %p\n",   list);
    ASSERT (funcs       != NULL, "Invalid argument funcs = %p\n",  funcs);

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    variables_t variables = {.var_num = 0, .var_table = {}};

    enum LangError result = kDoneLang;

    if (*node == NULL)
    {
        *node = TreeCtor ();
        CHECK_NULL_PTR (*node);
    }

    if (TOKEN_TYPE == kMainFunc)
    {
        (*node)->type = TOKEN_TYPE;
    }
    else
    {
        (*node)->type = kUserFunc;
        strcpy ((*node)->value.function.func_name, tokens [*token_index].value.variable);
    }
    SHIFT_TOKEN;

    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfFunc;
    }
    SHIFT_TOKEN;

    while (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
    {
        result = GetArgs (tokens, token_index, node, &variables);
        CHECK_RESULT ();
    }
    SHIFT_TOKEN;

    (*node)->value.function.cnt_loc_vars = (*node)->value.function.cnt_args;

    if (FindFuncInTable ((*node)->value.function, funcs) != funcs->func_num)
    {
        return kDoubleFuncDefinition;
    }

    if (!(CHECK_TOKEN_OP (kSym, kCurlyBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfFunc;
    }
    SHIFT_TOKEN;

    if ((*node)->left == NULL)
    {
        (*node)->left = TreeCtor ();
    }
    node_t** root = &((*node)->left);

    while (!(CHECK_TOKEN_OP (kSym, kCurlyBracketClose)))
    {
        result = GetCommand (tokens, token_index, root, list, &variables, *node);
        CHECK_RESULT ();
        root = &((*root)->left);
    }
    SHIFT_TOKEN;

    (*node)->value.function.func_num = funcs->func_num;
    funcs->func_table [funcs->func_num] = (*node)->value.function;
    funcs->func_num++;

    return result;
}

static enum LangError GetArgs (const token_t* const tokens, size_t* const token_index, node_t** const node,
                               variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (*node       != NULL, "Invalid argument *node = %p\n",       *node);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    if ((*node)->right == NULL)
    {
        (*node)->right = TreeCtor ();
    }
    node_t* root = (*node)->right;

    bool done = false;

    size_t count_args = 0;

    while (TOKEN_TYPE == kType)
    {
        done = true;
        root->type = TOKEN_TYPE;
        root->value.operation = tokens [*token_index].value.operation;
        SHIFT_TOKEN;

        root->right = AddNode ({.type = kVar, {.operation = kUndefinedNode}, .parent = NULL, .left = NULL, .right = NULL});
        CHECK_NULL_PTR (root->right);

        strcpy (variables->var_table [variables->var_num++], tokens [*token_index].value.variable);
        strcpy (root->right->value.variable.variable, tokens [*token_index].value.variable);
        root->right->value.variable.index = variables->var_num - 1;

        root = root->left = AddNode ({.type = kNewNode, {.operation = kUndefinedNode}, .parent = root, .left = NULL, .right = NULL});
        CHECK_NULL_PTR (root);
        SHIFT_TOKEN;

        if (!(CHECK_TOKEN_OP (kSym, kComma)) && !(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
        {
            SyntaxError (TOKEN_POSITION);
            return kMissCommaInArgs;
        }

        if (CHECK_TOKEN_OP (kSym, kComma))
        {
            SHIFT_TOKEN;
        }

        count_args++;
    }

    (*node)->value.function.cnt_args = count_args;

    if (done)
    {
        root->parent->left = NULL;
        TreeDtor (root);
    }

    return kDoneLang;
}

static enum LangError GetCommand (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                  list_t* const list, variables_t* const  variables, node_t* const func)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n"
                 "Current token val  = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    enum LangError result = kDoneLang;

    node_t* root = NULL;

    if (TOKEN_TYPE == kVar)
    {
        SHIFT_TOKEN;
        if (CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen))
        {
            REMOVE_TOKEN;
            result = GetFunctionCall (tokens, token_index, &root, list, variables);
        }
        else if (CHECK_TOKEN_OP (kSym, kAssign))
        {
            REMOVE_TOKEN;
            result = GetAssign (tokens, token_index, &root, list, variables);
            REMOVE_TOKEN;
        }
        else
        {
            REMOVE_TOKEN
            if (root != NULL)
            {
                TreeDtor (root);
            }
            return SyntaxError (TOKEN_POSITION);
        }
    }
    else if (TOKEN_TYPE == kType)
    {
        root = AddNode (TOKEN_OP_PATTERN);
        CHECK_NULL_PTR (root);
        SHIFT_TOKEN;
        strcpy (variables->var_table [variables->var_num++], tokens [*token_index].value.variable);
        long long index = FindVarInTable (tokens [*token_index].value.variable, *list, *variables);
        if (func->value.function.cnt_loc_vars < (size_t) index + 1) // В таблице нумерация идёт с нуля
        {
            func->value.function.cnt_loc_vars = (size_t) index + 1;
        }
        result = GetAssign (tokens, token_index, &(root->right), list, variables);
        REMOVE_TOKEN;
    }
    else if ((TOKEN_TYPE == kFunc) || (TOKEN_TYPE == kRet))
    {
        GetNumFunc (tokens, token_index, &root, list, variables);
    }
    else if (TOKEN_TYPE == kCycle)
    {
        root = AddNode (TOKEN_OP_PATTERN);
        CHECK_NULL_PTR (root);
        SHIFT_TOKEN;
        ListPushFront (list, variables);
        variables_t new_variables = {.var_num = 0, .var_table = {}};
        result = GetCycle (tokens, token_index, &(root), list, &new_variables, func);
        ListPopFront (list, &new_variables);
    }
    else if (TOKEN_TYPE == kCond)
    {
        root = AddNode (TOKEN_OP_PATTERN);
        CHECK_NULL_PTR (root);
        SHIFT_TOKEN;
        ListPushFront (list, variables);
        variables_t new_variables = {.var_num = 0, .var_table = {}};
        result = GetIf (tokens, token_index, &(root), list, &new_variables, func);
        CHECK_RESULT (TreeDtor (root););
        ListPopFront (list, &new_variables);
    }
    else
    {
        if (root != NULL)
        {
            TreeDtor (root);
        }
        return SyntaxError (TOKEN_POSITION);
    }

    if (result != kDoneLang)
    {
        if (root != NULL)
        {
            TreeDtor (root);
        }
        return result;
    }

    if (*node == NULL)
    {
        *node = AddNode ({.type = TOKEN_TYPE, {.operation = kCommandEnd}, .parent = NULL, .left = NULL, .right = root});
        CHECK_NULL_PTR (*node);
    }
    else
    {
        (**node) = {.type = TOKEN_TYPE, {.operation = kCommandEnd},  .parent = NULL, .left = NULL, .right = root};
    }

    if (!(CHECK_TOKEN_OP (kSym, kCommandEnd)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidCommand;
    }
    SHIFT_TOKEN;

    return result;
}

static enum LangError GetFunctionCall (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    if (TOKEN_TYPE != kVar)
    {
        return kInvalidTokenType;
    }

    if (*node == NULL)
    {
        *node = TreeCtor ();
    }

    (*node)->type = kUserFunc;
    strcpy ((*node)->value.function.func_name, tokens [*token_index].value.variable);
    SHIFT_TOKEN;

    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen)))
    {
        return kInvalidTokenType;
    }

    node_t** root = &((*node)->right);

    bool first_arg = true;

    size_t count_args = 0;

    while (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
    {
        if (!(first_arg) && !(CHECK_TOKEN_OP (kSym, kComma)))
        {
            return kMissCommaFuncCall;
        }

        first_arg = false;

        if (*root == NULL)
        {
            *root = TreeCtor ();
            if (*root == NULL)
            {
                return kCantCreateNode;
            }
        }

        **root = {.type = kSym, {.operation = kComma},  .parent = NULL, .left = NULL, .right = NULL};
        SHIFT_TOKEN;

        result = GetAddSub (tokens, token_index, &((*root)->right), list, variables);
        CHECK_RESULT ();

        root = &((*root)->left);
        count_args++;
    }
    SHIFT_TOKEN;

    (*node)->value.function.cnt_args = count_args;

    return kDoneLang;
}

static enum LangError GetAssign (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                 list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    if (TOKEN_TYPE != kVar)
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidAssigning;
    }
    SHIFT_TOKEN;
    if (!(CHECK_TOKEN_OP (kSym, kAssign)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidAssigning;
    }
    REMOVE_TOKEN;

    if (*node == NULL)
    {
        *node = ADDNODE_SYM (kAssign);
        CHECK_NULL_PTR (*node);
    }

    **node = {.type = kSym, {.operation = kAssign}, .parent = NULL, .left = NULL, .right = NULL};

    long long index = FindVarInTable (tokens [*token_index].value.variable, *list, *variables);

    (*node)->left = AddNode ({.type = tokens [*token_index].type, {.operation = kUndefinedNode},
                              .parent = NULL, .left = NULL, .right = NULL});
    CHECK_NULL_PTR ((*node)->left);
    if (index == LLONG_MAX)
    {
        SyntaxError (TOKEN_POSITION);
        return kUndefinedVariable;
    }

    strcpy ((*node)->left->value.variable.variable, tokens [*token_index].value.variable);
    (*node)->left->value.variable.index = index;

    SHIFT_TOKEN;
    SHIFT_TOKEN;

    enum LangError result = GetAddSub (tokens, token_index, &((*node)->right), list, variables);

    SHIFT_TOKEN;

    ConnectTree (*node);

    return result;
}

static enum LangError GetCycle (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                list_t* const list, variables_t* const variables, node_t* const func)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    node_t** root = &((*node)->right);

    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfCycle;
    }
    SHIFT_TOKEN;

    result = GetCond (tokens, token_index, root, list, variables);

    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfCycle;
    }
    SHIFT_TOKEN;

    if (!(CHECK_TOKEN_OP (kSym, kCurlyBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfCycle;
    }
    SHIFT_TOKEN;

    if ((*node)->left == NULL)
    {
        (*node)->left = TreeCtor ();
    }

    root = &((*node)->left);

    while (!(CHECK_TOKEN_OP (kSym, kCurlyBracketClose)))
    {
        result = GetCommand (tokens, token_index, root, list, variables, func);
        CHECK_RESULT ();
        root = &((*root)->left);
    }
    SHIFT_TOKEN;

    return result;
}

static enum LangError GetIf (const token_t* const tokens, size_t* const token_index, node_t** const node,
                             list_t* const list, variables_t* const variables, node_t* const func)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    if (*node == NULL)
    {
        REMOVE_TOKEN;
        *node = AddNode (TOKEN_OP_PATTERN);
        SHIFT_TOKEN;
    }
    else
    {
        REMOVE_TOKEN;
        **node = TOKEN_OP_PATTERN;
        SHIFT_TOKEN;
    }

    node_t** root = &((*node)->right);

    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfIf;
    }
    SHIFT_TOKEN;

    result = GetCond (tokens, token_index, root, list, variables);
    if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfIf;
    }
    SHIFT_TOKEN;

    if (!(CHECK_TOKEN_OP (kSym, kCurlyBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfIf;
    }
    SHIFT_TOKEN;

    if ((*node)->left == NULL)
    {
        (*node)->left = AddNode ({.type = kCond, {.operation = kElse}, .parent = NULL, .left = NULL, .right = NULL});
    }
    else
    {
        *((*node)->left) = {.type = kCond, {.operation = kElse}, .parent = NULL, .left = NULL, .right = NULL};
    }

    if ((*node)->left->left == NULL)
    {
        (*node)->left->left = TreeCtor ();
    }

    if ((*node)->left->right == NULL)
    {
        (*node)->left->right = TreeCtor ();
    }

    root = &((*node)->left->right);

    int i = 0;
    while (!(CHECK_TOKEN_OP (kSym, kCurlyBracketClose)))
    {
        result = GetCommand (tokens, token_index, root, list, variables, func);
        CHECK_RESULT ();
        root = &((*root)->left);
        i++;
    }
    SHIFT_TOKEN;

    if (!(CHECK_TOKEN_OP (kCond, kElse)))
    {
        TreeDtor ((*node)->left->left);
        (*node)->left->left = NULL;
        return result;
    }
    SHIFT_TOKEN;

    root = &((*node)->left->left);

    if (!(CHECK_TOKEN_OP (kSym, kCurlyBracketOpen)))
    {
        SyntaxError (TOKEN_POSITION);
        return kInvalidPatternOfIf;
    }
    SHIFT_TOKEN;

    while (!(CHECK_TOKEN_OP (kSym, kCurlyBracketClose)))
    {
        result = GetCommand (tokens, token_index, root, list, variables, func);
        CHECK_RESULT ();
        root = &((*root)->left);
    }
    SHIFT_TOKEN;

    return result;
}

static enum LangError GetCond (const token_t* const tokens, size_t* const token_index, node_t** const node,
                               list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);
    node_t** root = node;

    if (*root == NULL)
    {
        *root = AddNode ({.type = kNewNode, {.operation = kUndefinedNode}, .parent = NULL, .left = NULL, .right = NULL});
    }
    else
    {
        **root = {.type = kNewNode, {.operation = kUndefinedNode}, .parent = NULL, .left = NULL, .right = NULL};
    }

    GetAddSub (tokens, token_index, &((*root)->left), list, variables);
    if (TOKEN_TYPE != kComp)
    {
        return SyntaxError (TOKEN_POSITION);
    }
    node_t* tmp_node = (*root)->left;
    **root = TOKEN_OP_PATTERN;
    (*root)->left = tmp_node;
    SHIFT_TOKEN;
    GetAddSub (tokens, token_index, &((*root)->right), list, variables);

    return result;
}

static enum LangError GetAddSub (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                 list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    result = GetMulDiv (tokens, token_index, node, list, variables);
    CHECK_RESULT ();


    if (!(CHECK_TOKEN_OP (kArithm, kAdd)) && !(CHECK_TOKEN_OP (kArithm, kSub)))
    {
        return result;
    }

    CREATE_ROOT_WITH_NODE_IN_LEFT_SUBTREE_AND_MAKE_NODE_POINT_ON_ROOT;

    while (CHECK_TOKEN_OP (kArithm, kAdd) || CHECK_TOKEN_OP (kArithm, kSub))
    {
        PUT_ROOT_TO_THE_LEFT_SUBTREE;

        root->type = kArithm;

        root->value.operation = tokens [*token_index].value.operation;
        SHIFT_TOKEN;

        result = GetMulDiv (tokens, token_index, &(root->right), list, variables);
        root->right->parent = root;
        CHECK_RESULT (TreeDtor (root););

        *node = root;

        CHECK_END;
    }

    return result;
}

static enum LangError GetMulDiv (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                 list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    result = GetPow (tokens, token_index, node, list, variables);
    CHECK_RESULT ();

    if (!(CHECK_TOKEN_OP (kArithm, kMul)) && !(CHECK_TOKEN_OP (kArithm, kDiv)))
    {
        return result;
    }

    CREATE_ROOT_WITH_NODE_IN_LEFT_SUBTREE_AND_MAKE_NODE_POINT_ON_ROOT;

    while ((CHECK_TOKEN_OP (kArithm, kMul) || CHECK_TOKEN_OP (kArithm, kDiv)))
    {
        PUT_ROOT_TO_THE_LEFT_SUBTREE;

        root->type = kArithm;

        root->value.operation = tokens [*token_index].value.operation;
        SHIFT_TOKEN;

        result = GetPow (tokens, token_index, &(root->right), list, variables);
        root->right->parent = root;
        CHECK_RESULT (TreeDtor (root););

        if ((root->value.operation == kDiv)
            && ((root->right != NULL) && (root->right->type == kNum)
                && (abs (root->right->value.number - 0) < kAccuracy)))
        {
            SyntaxError (TOKEN_POSITION);
            return kCantDivideByZero;
        }

        *node = root;

        CHECK_END;
    }

    return result;
}

static enum LangError GetPow (const token_t* const tokens, size_t* const token_index, node_t** const node,
                              list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    result = GetBrace (tokens, token_index, node, list, variables);
    CHECK_RESULT ();

    if (!(CHECK_TOKEN_OP (kArithm, kPow)))
    {
        return result;
    }

    CREATE_ROOT_WITH_NODE_IN_LEFT_SUBTREE_AND_MAKE_NODE_POINT_ON_ROOT;

    while (CHECK_TOKEN_OP (kArithm, kPow))
    {
        PUT_ROOT_TO_THE_LEFT_SUBTREE;

        root->type = kArithm;

        root->value.operation = kPow;
        SHIFT_TOKEN;

        result = GetBrace (tokens, token_index, &(root->right), list, variables);
        CHECK_RESULT (TreeDtor (root););

        root->right->parent = root;

        *node = root;

        CHECK_END;
    }

    return result;
}

static enum LangError GetBrace (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    if (CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen))
    {
        SHIFT_TOKEN;

        result = GetAddSub (tokens, token_index, node, list, variables);
        CHECK_RESULT ();

        if (!(CHECK_TOKEN_OP (kSym, kParenthesesBracketClose)))
        {
            SyntaxError (TOKEN_POSITION);
            return kNoBraceCloser;
        }

        SHIFT_TOKEN;

        return result;
    }

    return GetNumFunc (tokens, token_index, node, list, variables);
}

static enum LangError GetNumFunc (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                  list_t* const list, variables_t* const variables)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n",      tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",        node);
    ASSERT (list        != NULL, "Invalid argument node = %p\n",        list);
    ASSERT (variables   != NULL, "Invalid argument variables = %p\n",   variables);

    enum LangError result = kDoneLang;

    LOG (kDebug, "Tokens' array      = %p\n"
                 "Index of token     = %lu\n"
                 "Node*              = %p\n"
                 "Current token type = %d\n",
                 tokens, *token_index, *node,
                 tokens [*token_index].type);

    if (*node == NULL)
    {
        *node = TreeCtor ();
    }

    (*node)->type = tokens [*token_index].type;
    if ((*node)->type == kNum)
    {
        (*node)->value.number = tokens [*token_index].value.number;
        SHIFT_TOKEN;
        return kDoneLang;
    }

    if ((*node)->type == kVar)
    {
        LOG (kDebug, "Variable in expression = \"%s\"\n", tokens [*token_index].value.variable);
        SHIFT_TOKEN;
        if (CHECK_TOKEN_OP (kSym, kParenthesesBracketOpen))
        {
            REMOVE_TOKEN;
            result = GetFunctionCall (tokens, token_index, node, list, variables);
            return result;
        }
        REMOVE_TOKEN;

        long long index = FindVarInTable (tokens [*token_index].value.variable, *list, *variables);
        if (index == LLONG_MAX)
        {
            return kUndefinedVariable;
        }
        strcpy ((*node)->value.variable.variable, tokens [*token_index].value.variable);
        (*node)->value.variable.index = index;
        SHIFT_TOKEN;

        return kDoneLang;
    }

    if (((*node)->type == kFunc) || ((*node)->type == kRet))
    {
        (*node)->value.operation = tokens [*token_index].value.operation;
        SHIFT_TOKEN;
        result = GetBrace (tokens, token_index, &((*node)->right), list, variables);
        return result;
    }

    return kInvalidTokenForExpression;
}

static long long FindVarInTable (const char* const var, list_t list, variables_t variables)
{
    ASSERT (var != NULL, "Invalid argument var = %p\n", var);

    LOG (kDebug, "Variable for finding = \"%s\"\n", var);

    variables_t variables_tmp = {.var_num = 0, .var_table = {}};
    long long index = (long long) (variables.var_num - 1);
    ListElemValLoad (&list, TailIndex (&list), &variables_tmp);
    long long global_var_num = variables_tmp.var_num;
    long long ret_val = LLONG_MAX;

    for (long long index_in_table = (long long) (variables.var_num - 1);
            (variables.var_num != 0) && (index_in_table >= 0); index_in_table--, index--)
    {
        if (strcmp (var, variables.var_table [index_in_table]) == 0)
        {
            return index;
        }
    }

    long long all_var_cnt = 0;
    size_t list_index = HeadIndex (&list);

    while (list_index != 0)
    {
        ListElemValLoad (&list, list_index, &variables_tmp);
        all_var_cnt += variables_tmp.var_num;
        for (long long index_in_table = (long long) (variables_tmp.var_num - 1);
             (ret_val == LLONG_MAX) && (variables_tmp.var_num != 0) && (index_in_table >= 0);
             index_in_table--, index--)
        {
            if (strcmp (var, variables_tmp.var_table [index_in_table]) == 0)
            {
                ret_val = index;
            }
        }
        list_index = PrevIndex (&list, list_index);
    }

    if (ret_val == LLONG_MAX)
    {
        return LLONG_MAX;
    }

    return ret_val + all_var_cnt - global_var_num;
}

static size_t FindFuncInTable (const func_node_t function, const funcs_t* const funcs)
{
    ASSERT (function.func_name != NULL, "Invalid argument func_name = %p\n", function.func_name);

    LOG (kDebug, "Function for finding = \"%s\"\n", function.func_name);

    for (size_t index = 0; index < funcs->func_num; index++)
    {
        if ((strcmp (function.func_name, funcs->func_table [index].func_name) == 0)
            && (function.cnt_args == funcs->func_table [index].cnt_args))
        {
            return index;
        }
    }

    return funcs->func_num;
}

static enum LangError ConnectFuncCall (node_t* const  node, const funcs_t* const funcs)
{
    ASSERT (node != NULL,  "Invalid argument NODE\n");
    ASSERT (funcs != NULL, "Invalid argument FUNCS\n");

    enum LangError result = kDoneLang;

    if (node->type == kUserFunc)
    {
        size_t index = FindFuncInTable (node->value.function, funcs);
        if (index == funcs->func_num)
        {
            return kUndefinedFuncForRead;
        }

        node->value.function.func_num = index;
    }

    if (node->left != NULL)
    {
        result = ConnectFuncCall (node->left, funcs);
        CHECK_RESULT ();
    }

    if (node->right != NULL)
    {
        result = ConnectFuncCall (node->right, funcs);
        CHECK_RESULT ();
    }

    return result;
}

static enum LangError SyntaxError (const size_t number_nl, const size_t line_pos)
{
    fprintf (stderr, "\n%sSyntax error:%s\n"
                     "%s:%lu:%lu\n\n",
                     kRedTextTerminal, kNormalTextTerminal,
                     sFileName, number_nl, line_pos);

    return kSyntaxError;
}

//-----DSL----------------------------------------------------------------------------------------------------

#undef TOKEN_POSITION
#undef SHIFT_TOKEN
#undef REMOVE_TOKEN
#undef CHECK_RESULT
#undef CHECK_TOKEN_OP
#undef TOKEN_TYPE
#undef CHECK_END
#undef TOKEN_OP_PATTERN
#undef CHECK_NULL_PTR

//------------------------------------------------------------------------------------------------------------
