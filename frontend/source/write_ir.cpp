#include "write_ir.h"

#include "language.h"
#include "struct_lang.h"
#include "dump_lang.h"      // For function EnumToStr

#define IR_FILE_ IR_file
extern "C" {
#include "libpyam_ir.h"
}

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

// #include "write_ir_dsl.h"

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

static enum LangError WriteGlobalVars          (const node_t* const root, FILE* const IR_file,
                                                size_t* const tmp_var_counter);
static size_t         CountGlobalVars          (const node_t* const root);
static enum LangError WriteGlobalVarsAssigning (const node_t* const root, FILE* const IR_file,
                                                size_t* const tmp_var_counter);

static enum LangError WriteAssign         (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteExpression     (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteCallFunc       (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteCallUserFunc   (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteFuncs          (const node_t* const root, FILE* const IR_file,
                                           size_t const tmp_var_counter);

static enum LangError WriteFuncPattern    (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter);

static enum LangError WriteArgs           (const node_t* const root, FILE* const IR_file, const size_t cnt_args);

static enum LangError WriteCommand        (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

static enum LangError WriteIf             (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

static enum LangError WriteCycle          (const node_t* const root, FILE* const IR_file,
                                           size_t* const tmp_var_counter, size_t* const label_counter);

static enum IR_SysCall_Indexes IdentifySysCall   (const node_t* const node);
static enum IrOpType           IdentifyOperation (const node_t* const node);

enum LangError WriteIR (const node_t* const root, FILE* const IR_file)
{
    ASSERT (root    != NULL, "Invalid argument root in WriteIR\n");
    ASSERT (IR_file != NULL, "Invalid argument IR_file in WriteIR\n");

    LOG (kDebug, "Root = %p\n"
                 "File = %p\n",
                 root, IR_file);

    size_t tmp_var_counter = 2;

    enum LangError result = WriteGlobalVars (root, IR_file, &tmp_var_counter);
    CHECK_RESULT;

    IR_CALL_MAIN_ (1LU);
    IR_GIVE_ARG_ (0LU, 1LU);
    IR_SYSCALL_ (0LU, kIR_SYS_CALL_ARRAY [SYSCALL_HLT_INDEX].Name,
                      kIR_SYS_CALL_ARRAY [SYSCALL_HLT_INDEX].NumberOfArguments);

    return WriteFuncs (root, IR_file, tmp_var_counter);
}

static enum LangError WriteGlobalVars (const node_t* const root, FILE* const IR_file,
                                       size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteGlobalVars\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteGlobalVars\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteGlobalVars\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    size_t cnt_global_vars = CountGlobalVars (root);
    IR_GLOBAL_VARS_NUM_ (cnt_global_vars);

    return WriteGlobalVarsAssigning (root, IR_file, tmp_var_counter);
}

static enum LangError WriteGlobalVarsAssigning (const node_t* const root, FILE* const IR_file,
                                                size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (CHECK_NODE_OP (root, kType, kDouble) && CHECK_NODE_OP (root->right, kSym, kAssign))
    {
        result = WriteAssign (root->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteGlobalVarsAssigning (root->left, IR_file, tmp_var_counter);
    }

    return result;
}

static size_t CountGlobalVars (const node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument root in CountGlobalVars\n");

    LOG (kDebug, "Root = %p\n", root);

    size_t cnt_global_vars = 0;

    if (CHECK_NODE_OP (root, kType, kDouble) && CHECK_NODE_OP (root->right, kSym, kAssign))
    {
        cnt_global_vars++;
    }

    if (root->left != NULL)
    {
        cnt_global_vars += CountGlobalVars (root->left);
    }

    return cnt_global_vars;
}

static enum LangError WriteAssign (const node_t* const root, FILE* const IR_file,
                                   size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteAssign\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteAssign\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteAssign\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if ((!CHECK_NODE_OP (root, kSym, kAssign)) || (!CHECK_NODE_TYPE (root->left, kVar)))
    {
        return kCantWriteAssigning;
    }

    result = WriteExpression (root->right, IR_file, tmp_var_counter);
    CHECK_RESULT;

    IR_ASSIGN_VAR_ (root->left->value.variable.index, *tmp_var_counter - 1, root->left->value.variable.variable);

    return result;
}

static enum LangError WriteExpression (const node_t* const root, FILE* const IR_file,
                                       size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteExpression\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteExpression\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteExpression\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    if (root->type == kUserFunc)
    {
        return WriteCallUserFunc (root, IR_file, tmp_var_counter);
    }

    if (root->type == kFunc)
    {
        return WriteCallFunc (root, IR_file, tmp_var_counter);
    }

    if (root->left != NULL)
    {
        result = WriteExpression (root->left, IR_file, tmp_var_counter);
        CHECK_RESULT;
    }

    size_t first_operand = *tmp_var_counter - 1;

    if (root->right != NULL)
    {
        result = WriteExpression (root->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
    }

    size_t second_operand = *tmp_var_counter - 1;

    if (root->type == kVar)
    {
        IR_ASSIGN_TMP_VAR_ (*tmp_var_counter, root->value.variable.index, root->value.variable.variable);
        (*tmp_var_counter)++;
        return kDoneLang;
    }

    if (root->type == kNum)
    {
        IR_ASSIGN_TMP_NUM_ (*tmp_var_counter, root->value.number);
        (*tmp_var_counter)++;
        return kDoneLang;
    }

    if ((root->type == kArithm) || (root->type == kComp))
    {
        enum IrOpType index = IdentifyOperation (root);
        if (index == IR_OP_TYPE_INVALID_OPERATION)
        {
            return kInvalidOperation;
        }
        IR_OPERATION_ (*tmp_var_counter, index, first_operand, second_operand);
        (*tmp_var_counter)++;
        return kDoneLang;
    }

    return kInvalidNodeTypeLangError;
}

static enum LangError WriteCallFunc (const node_t* const root, FILE* const IR_file,
                                     size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteCallFunc\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteCallFunc\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteCallFunc\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    result = WriteExpression (root->right, IR_file, tmp_var_counter);
    CHECK_RESULT;

    enum IR_SysCall_Indexes index = IdentifySysCall (root);
    if (index == INVALID_SYSCALL)
    {
        return kInvalidSyscall;
    }

    IR_GIVE_ARG_ (0LU, *tmp_var_counter - 1);
    IR_SYSCALL_ (*tmp_var_counter, kIR_SYS_CALL_ARRAY[index].Name,
                                       kIR_SYS_CALL_ARRAY[index].NumberOfArguments);
    (*tmp_var_counter)++;
    return result;
}

static enum LangError WriteCallUserFunc (const node_t* const root, FILE* const IR_file,
                                         size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteCallUserFunc\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteCallUserFunc\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteCallUserFunc\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    const node_t* var_node = root->right;

    size_t count_args = 0;

    size_t stk_args = 0;
    enum STACK_ERROR result_stk = STACK_INIT_SHORT (stk_args, kStackStartSize);
    if (result_stk != DONE)
    {
        return kCantCreateStackArgs;
    }

    while (CHECK_NODE_OP (var_node, kSym, kComma))
    {
        result = WriteExpression (var_node->right, IR_file, tmp_var_counter);
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
        IR_GIVE_ARG_ (arg_index, tmp_arg_index);
    }

    IR_CALL_FUNC_ (*tmp_var_counter, root->value.function.func_num, count_args, root->value.function.func_name);
    (*tmp_var_counter)++;

    STACK_DTOR_SHORT (stk_args);

    return result;
}

static enum LangError WriteFuncs (const node_t* const root, FILE* const IR_file,
                                  size_t const tmp_var_counter)
{
    ASSERT (root    != NULL, "Invalid argument root in WriteFuncs\n");
    ASSERT (IR_file != NULL, "Invalid argument IR_file in WriteFuncs\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, tmp_var_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = tmp_var_counter;

    if (CHECK_NODE_OP (root, kType, kDouble)
        && (CHECK_NODE_OP (root->right, kMainFunc, kMain) || CHECK_NODE_TYPE (root->right, kUserFunc)))
    {
        result = WriteFuncPattern (root->right, IR_file, &scope_tmp_var_counter);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteFuncs (root->left, IR_file, tmp_var_counter);
    }

    return kDoneLang;
}

static enum LangError WriteFuncPattern (const node_t* const root, FILE* const IR_file,
                                        size_t* const tmp_var_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteFuncPattern\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteFuncPattern\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteFuncPattern\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n",
                 root, IR_file, *tmp_var_counter);

    enum LangError result = kDoneLang;

    size_t label_counter = 0;
    *tmp_var_counter = 0;

    if (root->type == kUserFunc)
    {
        IR_FUNCTION_BODY_ (root->value.function.func_num,
                           root->value.function.cnt_args,
                           root->value.function.cnt_loc_vars,
                           root->value.function.func_name);
    }
    else if (root->type == kMainFunc)
    {
        IR_MAIN_BODY_ (root->value.function.cnt_loc_vars);
    }
    else
    {
        return kInvalidPatternOfFunc;
    }

    if (root->right != NULL)
    {
        result = WriteArgs (root->right, IR_file, root->value.function.cnt_args);
        CHECK_RESULT;
    }

    if (root->left != NULL)
    {
        return WriteCommand (root->left, IR_file, tmp_var_counter, &label_counter);
    }

    return kDoneLang;
}

static enum LangError WriteArgs (const node_t* const root, FILE* const IR_file, const size_t cnt_args)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteArgs\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteArgs\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n",
                 root, IR_file);

    const node_t* arg_node = root;

    size_t arg_index = 0;

    for (arg_index = 0;
         (arg_index < cnt_args) &&
         (CHECK_NODE_OP (arg_node, kType, kDouble)) && (CHECK_NODE_TYPE (arg_node->right, kVar));
         arg_index++)
    {
        IR_TAKE_ARG_ (arg_node->right->value.variable.index, cnt_args - arg_index - 1,
                      arg_node->right->value.variable.variable);
        arg_node = arg_node->left;
    }

    if (arg_index < cnt_args)
    {
        return kInvalidArgNum;
    }

    return kDoneLang;
}

static enum LangError WriteCommand (const node_t* const root, FILE* const IR_file,
                                    size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteCommand\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteCommand\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteCommand\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteCommand\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, IR_file, *tmp_var_counter, *label_counter);

    enum LangError result = kDoneLang;

    bool done = false;

    if (!CHECK_NODE_OP (root, kSym, kCommandEnd))
    {
        return kNoCommandEnd;
    }

    if (CHECK_NODE_OP (root->right, kSym, kAssign))
    {
        result = WriteAssign (root->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_OP (root->right, kType, kDouble))
    {
        result = WriteAssign (root->right->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kFunc))
    {
        result = WriteCallFunc (root->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_OP (root->right, kRet, kReturn))
    {
        result = WriteExpression (root->right->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
        IR_RET_ (*tmp_var_counter - 1);
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kUserFunc))
    {
        result = WriteCallUserFunc (root->right, IR_file, tmp_var_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kCond))
    {
        result = WriteIf (root->right, IR_file, tmp_var_counter, label_counter);
        CHECK_RESULT;
        done = true;
    }

    if (CHECK_NODE_TYPE (root->right, kCycle))
    {
        result = WriteCycle (root->right, IR_file, tmp_var_counter, label_counter);
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

    return WriteCommand (root->left, IR_file, tmp_var_counter, label_counter);
}

static enum LangError WriteIf (const node_t* const root, FILE* const IR_file,
                               size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteIf\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteIf\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteIf\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteIf\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, IR_file, *tmp_var_counter, *label_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = *tmp_var_counter;

    if (!(CHECK_NODE_OP (root, kCond, kIf) && CHECK_NODE_TYPE (root->right, kComp)
          && CHECK_NODE_OP (root->left, kCond, kElse)))
    {
        return kInvalidPatternOfIf;
    }

    IR_LABEL_ ((*label_counter)++, "If: comparison");

    result = WriteExpression (root->right, IR_file, &scope_tmp_var_counter);
    CHECK_RESULT;

    IR_COND_JMP_ (*label_counter, scope_tmp_var_counter - 1, "If: jump on \"then\" body");

    IR_JMP_ (*label_counter + 1, "If: jump over \"then\" body");

    IR_LABEL_ (*label_counter, "If: \"then\" body");
    size_t then_body_end_label_num = *label_counter + 1;
    *label_counter += 2;

    result = WriteCommand (root->left->right, IR_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    if (root->left->left == NULL)
    {
        IR_LABEL_ (then_body_end_label_num, "If: \"then\" body end");
        return kDoneLang;
    }

    size_t else_body_end_label_num = *label_counter;
    *label_counter += 1;

    IR_JMP_ (else_body_end_label_num, "If: jump over \"else\" body");

    IR_LABEL_ (then_body_end_label_num, "If: \"else\" body");

    result = WriteCommand (root->left->left, IR_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    IR_LABEL_ (else_body_end_label_num, "If: \"else\" body end");

    return kDoneLang;
}

static enum LangError WriteCycle (const node_t* const root, FILE* const IR_file,
                                  size_t* const tmp_var_counter, size_t* const label_counter)
{
    ASSERT (root            != NULL, "Invalid argument root in WriteCycle\n");
    ASSERT (IR_file         != NULL, "Invalid argument IR_file in WriteCycle\n");
    ASSERT (tmp_var_counter != NULL, "Invalid argument tmp_var_counter in WriteCycle\n");
    ASSERT (label_counter   != NULL, "Invalid argument label_counter in WriteCycle\n");

    LOG (kDebug, "Root          = %p\n"
                 "File          = %p\n"
                 "TMP counter   = %lu\n"
                 "Label counter = %lu\n",
                 root, IR_file, *tmp_var_counter, *label_counter);

    enum LangError result = kDoneLang;

    size_t scope_tmp_var_counter = *tmp_var_counter;

    if (!(CHECK_NODE_OP (root, kCycle, kWhile) && CHECK_NODE_TYPE (root->right, kComp)))
    {
        return kInvalidPatternOfCycle;
    }

    IR_LABEL_ ((*label_counter)++, "While: comparison");

    result = WriteExpression (root->right, IR_file, &scope_tmp_var_counter);
    CHECK_RESULT;

    IR_COND_JMP_ (*label_counter, scope_tmp_var_counter - 1, "While: jump at cycle body");

    IR_JMP_ (*label_counter + 1, "While: jump over cycle body");

    IR_LABEL_ (*label_counter, "While: body");
    const size_t while_body_end_label_num = *label_counter + 1;
    *label_counter += 2;

    result = WriteCommand (root->left, IR_file, &scope_tmp_var_counter, label_counter);
    CHECK_RESULT;

    IR_JMP_ (while_body_end_label_num - 2, "While: jump back to comparison");

    IR_LABEL_ (while_body_end_label_num, "While: end body");

    return kDoneLang;
}

static enum IR_SysCall_Indexes IdentifySysCall (const node_t* const node)
{
    ASSERT (node != NULL, "Invalid argument node\n");

    switch (node->value.operation)
    {
        case (kIn):
        {
            return SYSCALL_IN_INDEX;
        }
        case (kOut):
        {
            return SYSCALL_OUT_INDEX;
        }
        default:
            return INVALID_SYSCALL;
    }
}

static enum IrOpType IdentifyOperation (const node_t* const node)
{
    ASSERT (node != NULL, "Invalid argument node\n");

    switch (node->value.operation)
    {
        case (kAdd):
        {
            return IR_OP_TYPE_SUM;
        }
        case (kSub):
        {
            return IR_OP_TYPE_SUB;
        }
        case (kMul):
        {
            return IR_OP_TYPE_MUL;
        }
        case (kDiv):
        {
            return IR_OP_TYPE_DIV;
        }
        case (kEqual):
        {
            return IR_OP_TYPE_EQ;
        }
        case (kNEqual):
        {
            return IR_OP_TYPE_NEQ;
        }
        case (kLess):
        {
            return IR_OP_TYPE_LESS;
        }
        case (kLessOrEq):
        {
            return IR_OP_TYPE_LESSEQ;
        }
        case (kMore):
        {
            return IR_OP_TYPE_GREAT;
        }
        case (kMoreOrEq):
        {
            return IR_OP_TYPE_GREATEQ;
        }
        default:
            return IR_OP_TYPE_INVALID_OPERATION;
    }
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
#undef WRITE_ASSIGN_ARG_TO_VAR
#undef WRITE_ASSIGN_RES
#undef WRITE_ASSIGN_RES_TO_ARG
#undef WRITE_ASSIGN_TMP_TO_ARG
#undef WRITE_ASSIGN

#undef WRITE_CALL
#undef WRITE_FUNC

#undef WRITE_LABEL

#undef WRITE_COND_JMP
#undef WRITE_JMP

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
