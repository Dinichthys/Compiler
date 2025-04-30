#include "read_lang.h"

#include "parse_flags_lang.h"

static const char* sFileName = NULL;

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"
#include "list.h"

//-----DSL----------------------------------------------------------------------------------------------------

#define TOKEN_POSITION tokens [*token_index].number_of_line, tokens[*token_index].line_pos

#define SHIFT_TOKEN (*token_index)++;

#define REMOVE_TOKEN (*token_index)--;

#define CHECK_RESULT                \
        if (result != kDoneLang)    \
        {                           \
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

static enum LangError ReadDataBase        (const char* const input_file_name, node_t** const root);
static enum LangError ReadBufferFromFile (char** const  input_buf, const char* const input_file_name);
// static enum LangError ReadUserExpression  (node_t** const root, FILE* const dump_file);

static enum LangError ParseTokens (token_t** const tokens, const char* const input_file_name);

static enum LangError GetProgram (const token_t* const tokens, size_t* const token_index, node_t** const node);

static enum LangError GetFunctionPattern (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                          list_t* const list);
static enum LangError GetArgs         (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       variables_t* const variables);
static enum LangError GetCommand      (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetFunctionCall (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetAssign       (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetCycle        (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
static enum LangError GetIf           (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                       list_t* const list, variables_t* const variables);
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

static token_t IdentifyToken  (char* const word);
static long long FindVarInTable (const char* const var, list_t list, variables_t variables);

static void   SkipNumber  (const char* const input_buf, size_t* const offset);
static size_t SkipSpace   (const char* const input_buf, size_t* const counter_nl, size_t* const line_pos);

enum LangError ReadProgram (settings_of_program_t* const set, node_t** const root)
{
    ASSERT (set  != NULL, "Invalid argument set = %p\n",  set);
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    return ReadDataBase (set->input_file_name, root);
}

static enum LangError ReadDataBase (const char* const input_file_name, node_t** const root)
{
    ASSERT (input_file_name != NULL, "Invalid argument input file name = %p\n", input_file_name);
    ASSERT (root            != NULL, "Invalid argument root = %p\n",            root);

    sFileName = input_file_name;

    enum LangError result = kDoneLang;

    token_t* tokens = NULL;

    result = ParseTokens (&tokens, input_file_name);
    if (result != kDoneLang)
    {
        if (result == kSyntaxError)
        {
            FREE_AND_NULL (tokens);
        }
        return result;
    }

    size_t token_index = 0;

    result = GetProgram (tokens, &token_index, root);
    FREE_AND_NULL (tokens);
    if (result != kDoneLang)
    {
        return result;
    }
    ConnectTree (*root);

    return result;
}

static enum LangError ParseTokens (token_t** const tokens, const char* const input_file_name)
{
    ASSERT (tokens          != NULL, "Invalid argument tokens array = %p\n",       tokens);
    ASSERT (input_file_name != NULL, "Invalid argument Name of input file = %p\n", input_file_name);

    enum LangError result = kDoneLang;

    char* input_buf = NULL;

    result = ReadBufferFromFile (&input_buf, input_file_name);
    CHECK_RESULT

    *tokens = (token_t*) calloc (strlen (input_buf) + 1, sizeof (token_t));
    if (*tokens == NULL)
    {
        FREE_AND_NULL (input_buf);
        return kCantCallocTokenBuf;
    }

    size_t offset = 0;
    size_t token_index = 0;
    size_t counter_nl = 1;
    size_t line_pos = 1;
    bool in_comment = false;

    offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

    while (input_buf [offset] != kEOF)
    {
        LOG (kDebug, "Input buffer        = %p\n"
                     "Offset              = %lu\n"
                     "Position in line    = %lu\n"
                     "Number of new lines = %lu\n"
                     "Run time symbol     = {%c}\n",
                     input_buf, offset, line_pos, counter_nl, input_buf [offset]);

        if (input_buf [offset] == kCommentSymbol)
        {
            in_comment = !in_comment;
            offset++;
            line_pos++;
        }

        if (in_comment)
        {
            if (input_buf [offset] == '\n')
            {
                line_pos = 0;
                counter_nl++;
            }
            offset++;
            line_pos++;
            continue;
        }

        offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

        if ((isdigit (input_buf [offset])) || (input_buf [offset] == '-'))
        {
            double number = 0;
            sscanf (input_buf + offset, "%lf", &number);

            (*tokens) [token_index++] = {.type = kNum, {.number = number},
                                         .line_pos = line_pos, .number_of_line = counter_nl};

            size_t old_offset = offset;
            SkipNumber (input_buf, &offset);
            line_pos += offset - old_offset;

            offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

            continue;
        }

        LOG (kDebug, "Current symbol = {%c}\n", input_buf [offset]);

        char word [kWordLen] = "";
        sscanf (input_buf + offset, "%s", word);
        word [kWordLen - 1] = '\0';

        (*tokens) [token_index] = IdentifyToken (word);
        LOG (kDebug, "Length of the word = %lu\n", strlen (word));
        if ((*tokens) [token_index].type == kInvalidNodeType)
        {
            (*tokens) [token_index].type = kVar;
            strcpy ((*tokens) [token_index].value.variable, word);
        }
        (*tokens) [token_index].line_pos       = line_pos;
        (*tokens) [token_index].number_of_line = counter_nl;
        token_index++;

        offset += strlen (word);
        line_pos += strlen (word);

        offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

        continue;
    }

    FREE_AND_NULL (input_buf);

    (*tokens) [token_index].type = kEndToken;

    return kDoneLang;
}

static enum LangError ReadBufferFromFile (char** const  input_buf, const char* const input_file_name)
{
    ASSERT (input_buf       != NULL, "Invalid argument Input buffer addres = %p\n", input_buf);
    ASSERT (input_file_name != NULL, "Invalid argument Name of input file = %p\n",  input_file_name);

    FILE* input_file = NULL;
    FOPEN (input_file, input_file_name, "r");
    if (input_file == NULL)
    {
        return kCantOpenDataBase;
    }

    size_t file_size = size_of_file (input_file);

    LOG (kDebug, "Size of file = %lu\n",
                 file_size);

    if (file_size == 0)
    {
        return kCantReadDataBase;
    }

    *input_buf = (char*) calloc (file_size + 1, sizeof (char));
    if (*input_buf == NULL)
    {
        return kCantCallocInputBuffer;
    }

    LOG (kDebug, "Input buffer = %p\n"
                 "Size of file = %lu\n",
                 input_buf, file_size);

    if (fread (*input_buf, sizeof (char), file_size, input_file) == 0)
    {
        FREE_AND_NULL (*input_buf);
        return kCantReadDataBase;
    }
    FCLOSE (input_file);

    (*input_buf) [file_size] = kEOF;

    return kDoneLang;
}

static enum LangError GetProgram (const token_t* const tokens, size_t* const token_index, node_t** const node)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n", tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",   node);

    enum LangError result = kDoneLang;

    list_t list = {};

    variables_t variables = {.var_num = 0, .var_table = {}};

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
            result = GetFunctionPattern (tokens, token_index, &((*root)->right), &list);
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

    return result;
}

static enum LangError GetFunctionPattern (const token_t* const tokens, size_t* const token_index, node_t** const node,
                                          list_t* const list)
{
    ASSERT (token_index != NULL, "Invalid argument Index of the current token = %p\n", token_index);
    ASSERT (tokens      != NULL, "Invalid argument tokens = %p\n", tokens);
    ASSERT (node        != NULL, "Invalid argument node = %p\n",   node);
    ASSERT (list        != NULL, "Invalid argument list = %p\n",   list);

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
        CHECK_RESULT;
    }
    SHIFT_TOKEN;

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
        result = GetCommand (tokens, token_index, root, list, &variables);
        CHECK_RESULT;
        root = &((*root)->left);
    }
    SHIFT_TOKEN;

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
                                  list_t* const list, variables_t* const  variables)
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
        result = GetAssign (tokens, token_index, &(root->right), list, variables);
        REMOVE_TOKEN;
    }
    else if (TOKEN_TYPE == kFunc)
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
        result = GetCycle (tokens, token_index, &(root), list, &new_variables);
        ListPopFront (list, &new_variables);
    }
    else if (TOKEN_TYPE == kCond)
    {
        root = AddNode (TOKEN_OP_PATTERN);
        CHECK_NULL_PTR (root);
        SHIFT_TOKEN;
        ListPushFront (list, variables);
        variables_t new_variables = {.var_num = 0, .var_table = {}};
        result = GetIf (tokens, token_index, &(root), list, &new_variables);
        CHECK_RESULT;
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
        CHECK_RESULT;

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
        result = GetCommand (tokens, token_index, root, list, variables);
        CHECK_RESULT;
        root = &((*root)->left);
    }
    SHIFT_TOKEN;

    return result;
}

static enum LangError GetIf (const token_t* const tokens, size_t* const token_index, node_t** const node,
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
        result = GetCommand (tokens, token_index, root, list, variables);
        CHECK_RESULT;
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
        result = GetCommand (tokens, token_index, root, list, variables);
        CHECK_RESULT;
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
    CHECK_RESULT;


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
        CHECK_RESULT;

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
    CHECK_RESULT;

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
        CHECK_RESULT;

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
    CHECK_RESULT;

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
        CHECK_RESULT;

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
        CHECK_RESULT;

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

    if ((*node)->type == kFunc)
    {
        (*node)->value.operation = tokens [*token_index].value.operation;
        SHIFT_TOKEN;
        result = GetBrace (tokens, token_index, &((*node)->right), list, variables);
        return result;
    }

    return kInvalidTokenForExpression;
}

static token_t IdentifyToken (char* const word)
{
    ASSERT (word != NULL, "Invalid argument func = %p\n", word);

    LOG (kDebug, "Word = \"%s\"\n", word);

    for (size_t index = 0; index < kTokenNumber; index++)
    {
        if (strcmp (word, kTokenTypes [index].str_token) == 0)
        {
            return kTokenTypes [index].token;
        }
    }

    return {.type = kInvalidNodeType, {.operation = kInvalidFunc}, .line_pos = 0, .number_of_line = 0};
}

static long long FindVarInTable (const char* const var, list_t list, variables_t variables)
{
    ASSERT (var != NULL, "Invalid argument var = %p\n", var);

    LOG (kDebug, "Variable for finding = \"%s\"\n", var);

    long long index = (long long) (variables.var_num - 1);

    for (long long index_in_table = (long long) (variables.var_num - 1);
            (variables.var_num != 0) && (index_in_table >= 0); index_in_table--, index--)
    {
        if (strcmp (var, variables.var_table [index_in_table]) == 0)
        {
            return index;
        }
    }

    size_t list_index = HeadIndex (&list);

    variables_t variables_tmp = {.var_num = 0, .var_table = {}};

    while (list_index != 0)
    {
        ListElemValLoad (&list, list_index, &variables_tmp);
        for (long long index_in_table = (long long) (variables_tmp.var_num - 1);
             (variables_tmp.var_num != 0) && (index_in_table >= 0); index_in_table--, index--)
        {
            if (strcmp (var, variables_tmp.var_table [index_in_table]) == 0)
            {
                return index;
            }
        }
        list_index = PrevIndex (&list, list_index);
    }

    return LLONG_MAX;
}

static void SkipNumber (const char* const input_buf, size_t* const offset)
{
    ASSERT (input_buf != NULL, "Invalid argument input_buf = %p\n", input_buf);
    ASSERT (offset    != NULL, "Invalid argument offset = %p\n",    offset);

    LOG (kDebug, "Input buffer    = %p\n"
                 "Offset          = %lu\n"
                 "Run time symbol = {%c}\n",
                 input_buf, *offset, input_buf [*offset]);

    if (input_buf [*offset] == '-')
    {
        (*offset)++;
    }

    while (isdigit (input_buf [*offset]))
    {
        (*offset)++;
    }

    if (input_buf [*offset] == '.')
    {
        (*offset)++;
    }

    while (isdigit (input_buf [*offset]))
    {
        (*offset)++;
    }
}

static size_t SkipSpace (const char* const input_buf, size_t* const counter_nl, size_t* const line_pos)
{
    ASSERT (input_buf  != NULL, "Invalid argument: Pointer on input buffer = %p\n",         input_buf);
    ASSERT (counter_nl != NULL, "Invalid argument: Pointer on counter of new lines = %p\n", counter_nl);
    ASSERT (line_pos   != NULL, "Invalid argument: Pointer on line position = %p\n",        line_pos);

    size_t offset = 0;

    while (isspace (input_buf [offset]))
    {
        if (input_buf [offset] == '\n')
        {
            (*counter_nl)++;
            *line_pos = 0;
        }
        offset++;
        (*line_pos)++;
    }

    return offset;
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
