#include "write_ir.h"

#include <stdio.h>
#include <stdlib.h>

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

#include "Stack/stack.h"

//-----DSL----------------------------------------------------------------------------------------------------

//-----CHECKING-----------------------------------------------------------------------------------------------

#define CHECK_NODE_OP(node, node_type, node_op)                                         \
    ((node != NULL) && (node->type == node_type) && (node->value.operation == node_op))

#define CHECK_NODE_TYPE(node, node_type)          \
    ((node != NULL) && (node->type == node_type))

#define CHECK_RESULT                \
        if (result != kDoneLang)    \
        {                           \
            return result;          \
        }

//------------------------------------------------------------------------------------------------------------

//-----WRITING------------------------------------------------------------------------------------------------

#define WRITE_ASSIGN_VAR(variable)                                                                              \
    fprintf (ir_file, "GYPSI(88_tmp%lu, VAR, %s)              # var\n", *tmp_var_counter, variable);            \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_NUM(number)                                                                                \
    fprintf (ir_file, "GYPSI(88_tmp%lu, NUM, %lf)             # num\n", *tmp_var_counter, number);              \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP(number)                                                                                \
    fprintf (ir_file, "GYPSI(88_tmp%lu, TMP, 88_tmp%lu)       # tmp\n", *tmp_var_counter, number);              \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP_TO_VAR(var, number)                                                                    \
    fprintf (ir_file, "GYPSI(%s, TMP, 88_tmp%lu)              # tmp\n", var, number);

#define WRITE_ASSIGN_RES(variable)                                                                              \
    fprintf (ir_file, "GYPSI(%s, TMP, 88_tmp%lu)              # result\n", variable, *tmp_var_counter - 1);

#define WRITE_ASSIGN(operation, operand1, operand2)                                                             \
    fprintf (ir_file, "GYPSI(88_tmp%lu, %s, 88_tmp%lu, 88_tmp%lu)\n",                                           \
             *tmp_var_counter, operation, operand1, operand2);                                                  \
    (*tmp_var_counter)++;


#define WRITE_CALL(function, cnt_args)                                                                          \
    fprintf (ir_file, "CALL_PENIS(88_tmp%lu, func_%s_%lu, %lu)\n",                                              \
             *tmp_var_counter, function, cnt_args, cnt_args);                                                   \
    (*tmp_var_counter)++;

#define WRITE_FUNC(function, cnt_args)                                                                          \
    fprintf (ir_file, "\nPENIS(func_%s_%lu, %lu)\n\n",                                                          \
             function, cnt_args, cnt_args);


//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

static enum LangError WriteGlobalVars     (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteAssign         (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteExpression     (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteCallFunc       (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteCallUserFunc   (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteFuncs          (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteFuncPattern    (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteArgs           (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter, const size_t cnt_args);

static enum LangError WriteCommand        (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteIf             (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteCycle          (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

enum LangError WriteIR (const node_t* const root, FILE* const ir_file)
{
    ASSERT (root    != NULL, "Invalid argument root in WriteIR\n");
    ASSERT (ir_file != NULL, "Invalid argument ir_file in WriteIR\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n",
                 root, ir_file);

    size_t tmp_var_counter = 2;

    enum LangError result = WriteGlobalVars (root, ir_file, &tmp_var_counter);
    CHECK_RESULT;

    fprintf (ir_file, kStartIR);

    return WriteFuncs (root, ir_file, &tmp_var_counter);
}

static enum LangError WriteGlobalVars (const node_t* const root, FILE* const ir_file,
                                       size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (CHECK_NODE_OP (root, kType, kDouble) && CHECK_NODE_OP (root->right, kSym, kAssign))
    {
        result = WriteAssign (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteGlobalVars (root->left, ir_file, tmp_var_counter);
    }

    return result;
}

static enum LangError WriteAssign (const node_t* const root, FILE* const ir_file,
                                   size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if ((!CHECK_NODE_OP (root, kSym, kAssign)) || (!CHECK_NODE_TYPE (root->left, kVar)))
    {
        return kCantWriteAssigning;
    }

    result = WriteExpression (root->right, ir_file, tmp_var_counter);
    CHECK_RESULT;

    WRITE_ASSIGN_RES (root->left->value.variable.variable);

    return result;
}

static enum LangError WriteExpression (const node_t* const root, FILE* const ir_file,
                                       size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (root->type == kUserFunc)
    {
        return WriteCallUserFunc (root->right, ir_file, tmp_var_counter);
    }

    if (root->type == kFunc)
    {
        return WriteCallFunc (root->right, ir_file, tmp_var_counter);
    }

    if (root->left != NULL)
    {
        result = WriteExpression (root->left, ir_file, tmp_var_counter);
        CHECK_RESULT;
    }

    size_t first_operand = *tmp_var_counter - 1;

    if (root->right != NULL)
    {
        result = WriteExpression (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
    }

    size_t second_operand = *tmp_var_counter - 1;

    if (root->type == kVar)
    {
        WRITE_ASSIGN_VAR (root->value.variable.variable);
        return kDoneLang;
    }

    if (root->type == kNum)
    {
        WRITE_ASSIGN_NUM (root->value.number);
        return kDoneLang;
    }

    if ((root->type == kArithm) || (root->type == kComp))
    {
        WRITE_ASSIGN (EnumFuncToStr(root->value.operation), first_operand, second_operand);
        return kDoneLang;
    }

    return kInvalidNodeTypeLangError;
}

static enum LangError WriteCallFunc (const node_t* const root, FILE* const ir_file,
                                     size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    result = WriteExpression (root->right, ir_file, tmp_var_counter);
    CHECK_RESULT;

    WRITE_CALL (EnumFuncToStr(root->value.operation), 1LU);

    return result;
}

static enum LangError WriteCallUserFunc (const node_t* const root, FILE* const ir_file,
                                         size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    const node_t* var_node = root->right;

    size_t count_args = 0;

    size_t stk_args = 0;
    enum STACK_ERROR result_stk = STACK_INIT_SHORT (stk_args, kVarMaxNum);
    if (result_stk != DONE)
    {
        return kCantCreateStackArgs;
    }

    while (CHECK_NODE_OP (var_node, kSym, kComma))
    {
        result = WriteExpression (var_node->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        result_stk = stack_push (stk_args, *tmp_var_counter - 1);
        if (result_stk != DONE)
        {
            STACK_DTOR_SHORT (stk_args);
            return kCantPushTMPVarCounter;
        }
        count_args++;
        var_node = var_node->left;
    }

    size_t tmp_arg_index = 0;

    for (size_t arg_index = 0; arg_index < count_args; arg_index++)
    {
        result_stk = stack_pop (stk_args, &tmp_arg_index);
        if (result_stk != DONE)
        {
            STACK_DTOR_SHORT (stk_args);
            return kCantPopTMPVarCounter;
        }
        WRITE_ASSIGN_TMP (tmp_arg_index);
    }

    WRITE_CALL (root->value.function.func_name, count_args);

    STACK_DTOR_SHORT (stk_args);

    return result;
}

static enum LangError WriteFuncs (const node_t* const root, FILE* const ir_file,
                                  size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (CHECK_NODE_OP (root, kType, kDouble) && CHECK_NODE_OP (root->right, kMainFunc, kMain))
    {
        result = WriteFuncPattern (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
    }

    if (CHECK_NODE_OP (root, kType, kDouble) && CHECK_NODE_TYPE (root->right, kUserFunc))
    {
        result = WriteFuncPattern (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteFuncs (root->left, ir_file, tmp_var_counter);
    }

    return kDoneLang;
}

static enum LangError WriteFuncPattern (const node_t* const root, FILE* const ir_file,
                                        size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (root->type == kUserFunc)
    {
        WRITE_FUNC (root->value.function.func_name, root->value.function.cnt_args);
    }
    else if (root->type == kMainFunc)
    {
        WRITE_FUNC (kMainNodeWord, 0LU);
    }
    else
    {
        return kInvalidPatternOfFunc;
    }

    if (root->right != NULL)
    {
        result = WriteArgs (root->right, ir_file, tmp_var_counter, root->value.function.cnt_args);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteCommand (root->left, ir_file, tmp_var_counter);
    }

    return kDoneLang;
}

static enum LangError WriteArgs (const node_t* const root, FILE* const ir_file,
                                 size_t* const tmp_var_counter, const size_t cnt_args)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    const node_t* arg_node = root;

    size_t arg_index = 0;

    for (arg_index = 0;
         (arg_index < cnt_args) &&
         (CHECK_NODE_OP (arg_node, kType, kDouble)) && (CHECK_NODE_TYPE (arg_node->right, kVar));
         arg_index++)
    {
        WRITE_ASSIGN_TMP_TO_VAR (arg_node->right->value.variable.variable, *tmp_var_counter - arg_index - 2);
        arg_node = arg_node->left;
    }

    if (arg_index < cnt_args)
    {
        return kInvalidArgNum;
    }

    return kDoneLang;
}

static enum LangError WriteCommand (const node_t* const root, FILE* const ir_file,
                                    size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    bool done = false;

    if (!CHECK_NODE_OP (root, kSym, kCommandEnd))
    {
        return kNoCommandEnd;
    }

    if (CHECK_NODE_OP (root->right, kSym, kAssign))
    {
        result = WriteAssign (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_OP (root->right, kType, kDouble))
    {
        result = WriteAssign (root->right->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kFunc))
    {
        result = WriteCallFunc (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kUserFunc))
    {
        result = WriteCallUserFunc (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kCond))
    {
        result = WriteIf (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kCycle))
    {
        result = WriteCycle (root->right, ir_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (!done)
    {
        return kInvalidCommand;
    }

    if (root->left == NULL)
    {
        return kDoneLang;
    }

    return WriteCommand (root->left, ir_file, tmp_var_counter);
}

static enum LangError WriteIf (const node_t* const root, FILE* const ir_file,
                               size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    WRITE_FUNC (root->value.function.func_name, root->value.function.cnt_args);

    if (root->right != NULL)
    {
        result = WriteArgs (root->right, ir_file, tmp_var_counter, root->value.function.cnt_args);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteCommand (root->left, ir_file, tmp_var_counter);
    }

    return kDoneLang;
}

static enum LangError WriteCycle (const node_t* const root, FILE* const ir_file,
                                  size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n"
                 "TMP counter = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    WRITE_FUNC (root->value.function.func_name, root->value.function.cnt_args);

    if (root->right != NULL)
    {
        result = WriteArgs (root->right, ir_file, tmp_var_counter, root->value.function.cnt_args);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteCommand (root->left, ir_file, tmp_var_counter);
    }

    return kDoneLang;
}

//-----DSL----------------------------------------------------------------------------------------------------

#undef CHECK_RESULT

//------------------------------------------------------------------------------------------------------------
