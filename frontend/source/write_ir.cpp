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
    fprintf (ir_file, "\tGYPSI(88_tmp%lu, VAR, %s)              # var to tmp\n", *tmp_var_counter, variable);     \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_NUM(number)                                                                                \
    fprintf (ir_file, "\tGYPSI(88_tmp%lu, NUM, %lf)             # num to tmp\n", *tmp_var_counter, number);       \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP(number)                                                                                \
    fprintf (ir_file, "\tGYPSI(88_tmp%lu, TMP, 88_tmp%lu)       # tmp to tmp\n", *tmp_var_counter, number);       \
    (*tmp_var_counter)++;

#define WRITE_ASSIGN_TMP_TO_VAR(var, number)                                                                    \
    fprintf (ir_file, "\tGYPSI(%s, TMP, 88_tmp%lu)              # tmp to variable\n", var, number);

#define WRITE_ASSIGN_ARG_TO_VAR(var, arg_index)                                                                 \
    fprintf (ir_file, "\tGYPSI(%s, ARG, 14_arg%lu)              # arg to variable\n", var, arg_index);

#define WRITE_ASSIGN_RES(variable)                                                                              \
    fprintf (ir_file, "\tGYPSI(%s, TMP, 88_tmp%lu)              # result to variable\n",                          \
             variable, *tmp_var_counter - 1);

#define WRITE_ASSIGN_RES_TO_ARG(arg_index)                                                                      \
    fprintf (ir_file, "\tGYPSI(14_arg%lu, TMP, 88_tmp%lu)       # result to argument in function\n",              \
             arg_index, *tmp_var_counter - 1);

#define WRITE_ASSIGN_TMP_TO_ARG(arg_index, tmp_index)                                                           \
    fprintf (ir_file, "\tGYPSI(14_arg%lu, TMP, 88_tmp%lu)       # tmp to argument in function\n",                 \
             arg_index, tmp_index);

#define WRITE_ASSIGN(operation, operand1, operand2)                                                             \
    fprintf (ir_file, "\tGYPSI(88_tmp%lu, %s, 88_tmp%lu, 88_tmp%lu)\n",                                           \
             *tmp_var_counter, operation, operand1, operand2);                                                  \
    (*tmp_var_counter)++;


#define WRITE_CALL(function, cnt_args)                                                                          \
    fprintf (ir_file, "CALL_PENIS(88_tmp%lu, func_%s_%lu, %lu)\n",                                              \
             *tmp_var_counter, function, cnt_args, cnt_args);                                                   \
    (*tmp_var_counter)++;

#define WRITE_FUNC(function, cnt_args)                                                                          \
    fprintf (ir_file, "\nPENIS(func_%s_%lu, %lu)\n\n",                                                          \
             function, cnt_args, cnt_args);


#define WRITE_LABEL(label_num, meaning)                                                                         \
    fprintf (ir_file, "\nFIFT(label%lu)                          # " meaning "\n",                              \
             label_num);


#define WRITE_COND_JMP(label_num, tmp_num, meaning)                                                                      \
    fprintf (ir_file, "\nENTER(label%lu, 88_tmp%lu)              # " meaning "\n",                              \
             label_num, tmp_num);

#define WRITE_JMP(label_num, meaning)                                                                           \
    fprintf (ir_file, "\nENTER(label%lu)                         # " meaning "\n",                              \
             label_num);

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
                                           size_t const tmp_var_counter);

static enum LangError WriteFuncPattern    (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteArgs           (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter, const size_t cnt_args);

static enum LangError WriteCommand        (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

static enum LangError WriteIf             (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

static enum LangError WriteCycle          (const node_t* const root, FILE* const ir_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

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

    return WriteFuncs (root, ir_file, tmp_var_counter);
}

static enum LangError WriteGlobalVars (const node_t* const root, FILE* const ir_file,
                                       size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
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

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
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

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
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

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    result = WriteExpression (root->right, ir_file, tmp_var_counter);
    CHECK_RESULT;

    WRITE_ASSIGN_RES_TO_ARG (0LU);

    WRITE_CALL (EnumFuncToStr(root->value.operation), 1LU);

    return result;
}

static enum LangError WriteCallUserFunc (const node_t* const root, FILE* const ir_file,
                                         size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
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
        WRITE_ASSIGN_TMP_TO_ARG (arg_index, tmp_arg_index);
    }

    WRITE_CALL (root->value.function.func_name, count_args);

    STACK_DTOR_SHORT (stk_args);

    return result;
}

static enum LangError WriteFuncs (const node_t* const root, FILE* const ir_file,
                                  size_t const tmp_var_counter)
{
    ASSERT (root    != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file != NULL, "Invalid argument ir_file in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, ir_file, tmp_var_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = tmp_var_counter;

    if (CHECK_NODE_OP (root, kType, kDouble)
        && (CHECK_NODE_OP (root->right, kMainFunc, kMain) || CHECK_NODE_TYPE (root->right, kUserFunc)))
    {
        result = WriteFuncPattern (root->right, ir_file, &scope_tmp_var_counter);
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

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, ir_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    size_t label_counter = 0;

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
        return WriteCommand (root->left, ir_file, tmp_var_counter, &label_counter);
    }

    return kDoneLang;
}

static enum LangError WriteArgs (const node_t* const root, FILE* const ir_file,
                                 size_t* const tmp_var_counter, const size_t cnt_args)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, ir_file, *tmp_var_counter);

    const node_t* arg_node = root;

    size_t arg_index = 0;

    for (arg_index = 0;
         (arg_index < cnt_args) &&
         (CHECK_NODE_OP (arg_node, kType, kDouble)) && (CHECK_NODE_TYPE (arg_node->right, kVar));
         arg_index++)
    {
        WRITE_ASSIGN_ARG_TO_VAR (arg_node->right->value.variable.variable, cnt_args - arg_index - 1);
        arg_node = arg_node->left;
    }

    if (arg_index < cnt_args)
    {
        return kInvalidArgNum;
    }

    return kDoneLang;
}

static enum LangError WriteCommand (const node_t* const root, FILE* const ir_file,
                                    size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, ir_file, *tmp_var_counter, *label_counter);

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
        result = WriteIf (root->right, ir_file, tmp_var_counter, label_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kCycle))
    {
        result = WriteCycle (root->right, ir_file, tmp_var_counter, label_counter);
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

    return WriteCommand (root->left, ir_file, tmp_var_counter, label_counter);
}

static enum LangError WriteIf (const node_t* const root, FILE* const ir_file,
                               size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, ir_file, *tmp_var_counter, *label_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = *tmp_var_counter;

    if (!(CHECK_NODE_OP (root, kCond, kIf) && CHECK_NODE_TYPE (root->right, kComp)
          && CHECK_NODE_OP (root->left, kCond, kElse)))
    {
        return kInvalidPatternOfIf;
    }

    WRITE_LABEL ((*label_counter)++, "If: comparison");

    result = WriteExpression (root->right, ir_file, &scope_tmp_var_counter);
    CHECK_RESULT;

    WRITE_COND_JMP (*label_counter, scope_tmp_var_counter - 1, "If: jump on \"then\" body");

    WRITE_JMP (*label_counter + 1, "If: jump over \"then\" body");

    WRITE_LABEL (*label_counter, "If: \"then\" body");
    size_t then_body_end_label_num = *label_counter + 1;
    *label_counter += 2;

    result = WriteCommand (root->left->right, ir_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    if (root->left->left == NULL)
    {
        WRITE_LABEL (then_body_end_label_num, "If: \"then\" body end");
        return kDoneLang;
    }

    size_t else_body_end_label_num = *label_counter;
    *label_counter += 1;

    WRITE_JMP (else_body_end_label_num, "If: jump over \"else\" body");

    WRITE_LABEL (then_body_end_label_num, "If: \"else\" body");

    result = WriteCommand (root->left->left, ir_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    WRITE_LABEL (else_body_end_label_num, "If: \"else\" body end");

    return kDoneLang;
}

static enum LangError WriteCycle (const node_t* const root, FILE* const ir_file,
                                  size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (ir_file         != NULL, "Invalid argument ir_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, ir_file, *tmp_var_counter, *label_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = *tmp_var_counter;

    if (!(CHECK_NODE_OP (root, kCycle, kWhile) && CHECK_NODE_TYPE (root->right, kComp)))
    {
        return kInvalidPatternOfCycle;
    }

    WRITE_LABEL ((*label_counter)++, "While: comparison");

    result = WriteExpression (root->right, ir_file, &scope_tmp_var_counter);
    CHECK_RESULT;

    WRITE_COND_JMP (*label_counter, scope_tmp_var_counter - 1, "While: jump at cycle body");

    WRITE_JMP (*label_counter + 1, "While: jump over cycle body");

    WRITE_LABEL (*label_counter, "While: body");
    const size_t while_body_end_label_num = *label_counter + 1;
    *label_counter += 2;

    result = WriteCommand (root->left, ir_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    WRITE_JMP (while_body_end_label_num - 2, "While: jump back to comparison");

    WRITE_LABEL (while_body_end_label_num, "While: end body");

    return kDoneLang;
}

//-----DSL----------------------------------------------------------------------------------------------------

//-----CHECKING-----------------------------------------------------------------------------------------------

#undef CHECK_NODE_OP
#undef CHECK_NODE_TYPE
#undef CHECK_RESULT

//------------------------------------------------------------------------------------------------------------

//-----WRITING------------------------------------------------------------------------------------------------

#undef WRITE_ASSIGN_VAR
#undef WRITE_ASSIGN_NUM
#undef WRITE_ASSIGN_TMP
#undef WRITE_ASSIGN_TMP_TO_VAR
#undef WRITE_ASSIGN_RES
#undef WRITE_ASSIGN

#undef WRITE_CALL
#undef WRITE_FUNC

#undef WRITE_LABEL

#undef WRITE_COND_JMP
#undef WRITE_JMP

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
