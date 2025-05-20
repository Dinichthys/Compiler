#include "translationNASM.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "language.h"
#include "read_ir.h"

#include "list.h"
#include "list_src.h"
#include "list_construction.h"

#include "key_words.h"

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

typedef struct current_position
{
    const list_t* const list;
    size_t              index;
    IRInstruction_t     instruction;
    size_t              global_vars_cnt;
    size_t              cnt_func_args;
    size_t              max_tmp_counter;
    size_t              cnt_loc_vars;
} current_position_t;

static enum LangError GenerateAsmNASM (const list_t* const list, FILE* const output_file);

static enum LangError PrintStartProgram (FILE* const output_file);

static enum LangError CallFuncNASM      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError FuncBodyNASM      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError CondJumpNASM      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignNASM        (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError AssignVarNASM     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignTmpNASM     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignArgNASM     (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError OperationNASM     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError LabelNASM         (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError ReturnNASM        (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError SysCallNASM       (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError GlobalVarsNumNASM (current_position_t* const cur_pos, FILE* const output_file);

static size_t MaxTmpIndexBeforeCall (const current_position_t* const cur_pos);
static size_t IndexSysCall      (const char* const syscall_name);

enum LangError GenerateAsmNASMFromIR (FILE* const ir_file, FILE* const output_file)
{
    ASSERT (ir_file     != NULL, "Invalid argument ir_file in function GenerateASMNASMFromIR\n");
    ASSERT (output_file != NULL, "Invalid argument output_file in function GenerateASMNASMFromIR\n");

    char* buffer = ReadFileToBuffer (ir_file);
    if (buffer == NULL)
    {
        return kCantReadDataBase;
    }

    list_t list = {};
    enum LangError result = ReadIR (buffer, &list);
    FREE_AND_NULL (buffer);
    if (result != kDoneLang)
    {
        ListDtor (&list);
        return result;
    }

    result = GenerateAsmNASM (&list, output_file);
    ListDtor (&list);
    return result;
}

static enum LangError GenerateAsmNASM (const list_t* const list, FILE* const output_file)
{
    ASSERT (list        != NULL, "Invalid argument list\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    enum LangError result = kDoneLang;

    current_position_t cur_pos =
    {
        .list            = list,
        .index           = 0,
        .instruction     = {},
        .global_vars_cnt = 0,
        .cnt_func_args   = 0,
        .max_tmp_counter = 0,
        .cnt_loc_vars    = 0,
    };
    cur_pos.index = NextIndex (list, cur_pos.index);

    PrintStartProgram (output_file);

    while (cur_pos.index != 0)
    {
        ListElemValLoad (list, cur_pos.index, &(cur_pos.instruction));
        switch (cur_pos.instruction.type)
        {
            case IR_FUNCTION_CALL_INDEX:
            {
                result = CallFuncNASM (&cur_pos, output_file);
                break;
            }
            case IR_FUNCTION_BODY_INDEX:
            {
                result = FuncBodyNASM (&cur_pos, output_file);
                break;
            }
            case IR_CONDITIONAL_JUMP_INDEX:
            {
                result = CondJumpNASM (&cur_pos, output_file);
                break;
            }
            case IR_ASSIGNMENT_INDEX:
            {
                result = AssignNASM (&cur_pos, output_file);
                break;
            }
            case IR_LABEL_INDEX:
            {
                result = LabelNASM (&cur_pos, output_file);
                break;
            }
            case IR_SYSTEM_FUNCTION_CALL_INDEX:
            {
                result = SysCallNASM (&cur_pos, output_file);
                break;
            }
            case IR_OPERATION_INDEX:
            {
                result = OperationNASM (&cur_pos, output_file);
                break;
            }
            case IR_RETURN_INDEX:
            {
                result = ReturnNASM (&cur_pos, output_file);
                break;
            }
            case IR_GLOBAL_VARS_NUM_INDEX:
            {
                result = GlobalVarsNumNASM (&cur_pos, output_file);
                break;
            }
            case IR_INVALID_KEY_WORD:
            default:
            {
                return kInvalidKeyWordIR;
            }
            if (result != kDoneLang)
            {
                return result;
            }
        }
        cur_pos.index = NextIndex (list, cur_pos.index);
    }

    return kDoneLang;
}

static enum LangError PrintStartProgram (FILE* const output_file)
{
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    fprintf (output_file, "section .text\n\n"
                          "global %s\n\n",
                          kNASMStartLabelName);

    for (size_t syscall_index = 0; syscall_index < kIR_SYS_CALL_NUMBER; syscall_index++)
    {
        fprintf (output_file, "extern %s_syscall\n", kIR_SYS_CALL_ARRAY [syscall_index].Name);
    }

    return kDoneLang;
}

static enum LangError CallFuncNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "CallFunc started analyzing\n");

    LOG (kDebug, "Calling function is \"%s\"\n", cur_pos->instruction.label);

    size_t cnt_call_func_args = 0;
    if (strcmp (cur_pos->instruction.label, kNASMMainFuncName) != 0)
    {
        sscanf (strchr (strchr (cur_pos->instruction.label, '_') + 1, '_'), "_%lu", &cnt_call_func_args);
    }

    fprintf (output_file, "\tsub %s, %lu\t ; Shift tmp counter\n"
                          "\tsub %s, %lu\t ; Shift RSP\n"
                          "call %s\t ; Calling function\n"
                          "\tadd %s, %lu\t ; Shift back RSP\n"
                          "\tadd %s, %lu\t ; Shift back tmp counter\n\n"
                          "\tmov qword [%s-%lu], %s\t ; Save return value to tmp var\n\n",
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          (cur_pos->max_tmp_counter + cnt_call_func_args + 1 + kNASMSavedRegistersNum + 1) * kNASMQWordSize,
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->max_tmp_counter + cnt_call_func_args) * kNASMQWordSize,
                          cur_pos->instruction.label,
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->max_tmp_counter + cnt_call_func_args) * kNASMQWordSize,
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          (cur_pos->max_tmp_counter + cnt_call_func_args + 1 + kNASMSavedRegistersNum + 1) * kNASMQWordSize,
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          cur_pos->instruction.res_index * kNASMQWordSize,
                          kNASMRegisters [kNASMRetValRegIndex]);

    if (cur_pos->instruction.res_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.res_index;
    }

    return kDoneLang;
}

static enum LangError FuncBodyNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "FuncBody started analyzing\n");

    cur_pos->cnt_func_args = cur_pos->instruction.value_1.number_size_t;
    cur_pos->cnt_loc_vars  = cur_pos->instruction.value_2.number_size_t;

    LOG (kDebug, "Start function \"%s\" body\n", cur_pos->instruction.label);

    fprintf (output_file, "\n%s:\t ; Function body\n"
                          "\tpush %s\t ; Save variables counter register value\n"
                          "\tpush %s\t ; Save tmp variables counter register value\n"
                          "\tpush %s\t ; Save RBP\n\n"
                          "\tmov %s, %s\t ; Move tmp var counter reg value to the var counter reg\n\n"
                          "\tsub %s, %lu\t ; Make a stack frame for local variables\n\n"
                          "\tmov %s, %s\t ; Save old RSP value\n"
                          "\tadd %s, %lu\n"
                          "\tmov %s, %s\t ; Make RBP point on the first arg\n"
                          "\tmov %s, %s\t ; Save old RSP value\n"
                          "\tsub %s, %lu\t ; Make RSP == Tmp counter\n",
                          cur_pos->instruction.label,
                          kNASMRegisters [kNASMVarBaseRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMRBPRegIndex],
                          kNASMRegisters [kNASMVarBaseRegIndex], kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          cur_pos->instruction.value_2.number_size_t * kNASMQWordSize,
                          kNASMRegisters [kNASMUnusedRegisterIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMRSPRegIndex],
                          (kNASMSavedRegistersNum + cur_pos->cnt_func_args) * kNASMQWordSize,
                          kNASMRegisters [kNASMRBPRegIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMRSPRegIndex], kNASMRegisters [kNASMUnusedRegisterIndex],
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->instruction.value_2.number_size_t + 1) * kNASMQWordSize);
                          // В стеке лежит 3 сохранённых регистра и адрес возврата, а затем аргументы

    return kDoneLang;
}

static enum LangError CondJumpNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "CondJump started analyzing\n");

    bool cond_jump = (cur_pos->instruction.type_1 != kNum);
    if (cond_jump)
    {
        if (cur_pos->instruction.value_1.operand_1_index > cur_pos->max_tmp_counter)
        {
            cur_pos->max_tmp_counter = cur_pos->instruction.value_1.operand_1_index;
        }
    }

    if (cond_jump)
    {
        fprintf (output_file, "\tmov %s, qword [%s-%lu]\n"
                              "\ttest %s, %s\t ; Compare the result of the condition\n"
                              "jne %s\t ; Jump on label if the condition is true\n",
                              kNASMRegisters [kNASMUnusedRegisterIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                              kNASMRegisters [kNASMUnusedRegisterIndex],
                              kNASMRegisters [kNASMUnusedRegisterIndex],
                              cur_pos->instruction.label);
    }
    else
    {
        fprintf (output_file, "jmp %s\t ; Jump on label\n", cur_pos->instruction.label);
    }

    return kDoneLang;
}

static enum LangError AssignNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Assigning started analyzing\n");

    switch (cur_pos->instruction.res_type)
    {
        case kGlobalVar:
        case kVar: return AssignVarNASM (cur_pos, output_file);
        case kTmp: return AssignTmpNASM (cur_pos, output_file);
        case kArg: return AssignArgNASM (cur_pos, output_file);
        default:   return kInvalidPrefixIR;
    }
}

static enum LangError AssignVarNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Assigning to var started analyzing\n");

    switch (cur_pos->instruction.type_1)
    {
        case kTmp:
        {
            if (cur_pos->instruction.value_1.operand_1_index > cur_pos->max_tmp_counter)
            {
                cur_pos->max_tmp_counter = cur_pos->instruction.value_1.operand_1_index;
            }

            fprintf (output_file, "\tmov %s, qword [%s-%lu]\t ; Push tmp to variable through the register\n"
                                  "\tmov qword [%s-%lu], %s\n\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMTmpBaseRegIndex],
                                  cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                                  (cur_pos->instruction.res_type == kVar)
                                                                  ? kNASMRegisters [kNASMVarBaseRegIndex]
                                                                  : kNASMRegisters [kNASMGlobalVarsStartIndex],
                                  cur_pos->instruction.res_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex]);

            return kDoneLang;
        }
        case kArg:
        {
            fprintf (output_file, "\tmov %s, %s\t ; Save RBP for one memory access\n"
                                  "\tsub %s, %lu\t ; Save value temporary to RBP\n"
                                  "\tmov %s, qword [%s]\t ; Push argument to variable through the register\n"
                                  "\tmov qword [%s-%lu], %s\n"
                                  "\tmov %s, %s\t ; Save value to RBP\n\n",
                                  kNASMRegisters [kNASMSavingRegister], kNASMRegisters [kNASMRBPRegIndex],
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex], kNASMRegisters [kNASMRBPRegIndex],
                                  (cur_pos->instruction.res_type == kVar)
                                                                  ? kNASMRegisters [kNASMVarBaseRegIndex]
                                                                  : kNASMRegisters [kNASMGlobalVarsStartIndex],
                                  cur_pos->instruction.res_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMRBPRegIndex], kNASMRegisters [kNASMSavingRegister]);

            return kDoneLang;
        }
        default:
            return kInvalidPrefixIR;
    }

}

static enum LangError AssignTmpNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Assigning to tmp started analyzing\n");

    if (cur_pos->instruction.res_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.res_index;
    }

    switch (cur_pos->instruction.type_1)
    {
        case kNum:
        {
            fprintf (output_file, "\tmov %s, 0x%016lx\n"
                                  "\tmov qword [%s-%lu], %s\t ; Push the number %lg to tmp through the stack\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  *((unsigned long int*) (&cur_pos->instruction.value_1.number_double)),
                                  kNASMRegisters [kNASMTmpBaseRegIndex],
                                  cur_pos->instruction.res_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  cur_pos->instruction.value_1.number_double);

            return kDoneLang;
        }
        case kVar:
        {
            fprintf (output_file, "\tmov %s, qword [%s-%lu]\t ; Push variable to tmp through the register\n"
                                  "\tmov qword [%s-%lu], %s\n\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  (cur_pos->instruction.type_1 == kVar)
                                                                  ? kNASMRegisters [kNASMVarBaseRegIndex]
                                                                  : kNASMRegisters [kNASMGlobalVarsStartIndex],
                                  cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMTmpBaseRegIndex],
                                  cur_pos->instruction.res_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex]);

            return kDoneLang;
        }
        case kTmp:
        {
            if (cur_pos->instruction.value_1.operand_1_index > cur_pos->max_tmp_counter)
            {
                cur_pos->max_tmp_counter = cur_pos->instruction.value_1.operand_1_index;
            }

            fprintf (output_file, "\tmov %s, qword [%s-%lu]\t ; Push tmp to tmp through the register\n"
                                  "\tmov qword [%s-%lu], %s\n\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMTmpBaseRegIndex],
                                  cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMTmpBaseRegIndex],
                                  cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex]);

            return kDoneLang;
        }
        default:
            return kInvalidAssigning;
    }
}

static enum LangError AssignArgNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Assigning to arg started analyzing\n");

    if (cur_pos->instruction.value_1.operand_1_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.value_1.operand_1_index;
    }

    fprintf (output_file, "\tmov %s, qword [%s-%lu]\n"
                          "\tmov qword [%s-%lu], %s\t ; Push tmp %lu to argument %lu through the register\n",
                          kNASMRegisters [kNASMUnusedRegisterIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          (MaxTmpIndexBeforeCall (cur_pos) + cur_pos->instruction.res_index + 1) * kNASMQWordSize,
                          kNASMRegisters [kNASMUnusedRegisterIndex],
                          cur_pos->instruction.value_1.operand_1_index,
                          cur_pos->instruction.res_index);

    return kDoneLang;
}

static size_t MaxTmpIndexBeforeCall (const current_position_t* const cur_pos)
{
    ASSERT(cur_pos != NULL, "Invalid argument cur_pos\n");

    size_t max_tmp_counter = cur_pos->max_tmp_counter;
    size_t index = cur_pos->index;
    IRInstruction_t instruction = {};

    if (index == 0)
    {
        return max_tmp_counter;
    }

    ListElemValLoad (cur_pos->list, index, &instruction);
    while ((index != 0) && (instruction.type != IR_FUNCTION_CALL_INDEX)
                        && (instruction.type != IR_SYSTEM_FUNCTION_CALL_INDEX))
    {
        if ((instruction.res_type == kTmp) && (instruction.res_index > max_tmp_counter))
        {
            max_tmp_counter = instruction.res_index;
        }
        if ((instruction.type_1 == kTmp) && (instruction.value_1.operand_1_index > max_tmp_counter))
        {
            max_tmp_counter = instruction.value_1.operand_1_index;
        }
        if ((instruction.type_2 == kTmp) && (instruction.value_2.operand_2_index > max_tmp_counter))
        {
            max_tmp_counter = instruction.value_2.operand_2_index;
        }

        index = NextIndex (cur_pos->list, index);
        ListElemValLoad (cur_pos->list, index, &instruction);
    }

    return max_tmp_counter;
}

static enum LangError OperationNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Operation started analyzing\n");

    if (cur_pos->instruction.res_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.res_index;
    }

    if (cur_pos->instruction.value_1.operand_1_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.value_1.operand_1_index;
    }

    if (cur_pos->instruction.value_2.operand_2_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.value_2.operand_2_index;
    }

    if (((size_t) cur_pos->instruction.operation) < kNASMIRArithmOperationsNum)
    {
        fprintf (output_file, "\tmovq %s, qword [%s-%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s-%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\n"
                              "\tmovq qword [%s-%lu], %s\t ; Move the result to the tmp var\n\n",
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.value_2.operand_2_index * kNASMQWordSize,
                              kInstructionsArithmNASM [cur_pos->instruction.operation - 1], // Енамы нумеруются с 1, а элементы массива с 0
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.res_index * kNASMQWordSize, kNASMRegisters [kNASMFirstOpRegIndex]);
    }
    else
    {
        fprintf (output_file, "\tmovq %s, qword [%s-%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s-%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\t ; Comparison\n"
                              "\tmovq %s, %s\t ; Move the result of comparison to the unused reg\n"
                              "\tshr %s, %lu\t ; Make the result 1 or 0\n"
                              "\tmov qword [%s-%lu], %s\t ; Move the result to the tmp var\n\n",
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.value_1.operand_1_index * kNASMQWordSize,
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.value_2.operand_2_index * kNASMQWordSize,
                              kInstructionsLogicNASM [cur_pos->instruction.operation - kNASMIRArithmOperationsNum - 1],
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMUnusedRegisterIndex], kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMUnusedRegisterIndex], kRegisterSize - 1,
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.res_index * kNASMQWordSize, kNASMRegisters [kNASMUnusedRegisterIndex]);
    }

    return kDoneLang;
}

static enum LangError LabelNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Label started analyzing\n");

    LOG (kDebug, "Label \"%s\" was analyzed\n", cur_pos->instruction.label);

    fprintf (output_file, "%s:\t ; Label in function\n", cur_pos->instruction.label);

    return kDoneLang;
}

static enum LangError ReturnNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Return started analyzing\n");

    if (cur_pos->instruction.res_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.res_index;
    }

    fprintf (output_file, "\n\tmov %s, qword [%s-%lu]\t ; Save return value to the rax\n"
                          "\tadd %s, %lu\t ; Get back RSP\n"
                          "\tpop %s\t ; Get RBP value back\n"
                          "\tpop %s\t ; Get tmp register value back\n"
                          "\tpop %s\t ; Get var register value back\n"
                          "ret\n",
                          kNASMRegisters [kNASMRetValRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          cur_pos->instruction.res_index * kNASMQWordSize,
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->cnt_loc_vars + 1) * kNASMQWordSize,
                          kNASMRegisters [kNASMRBPRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMVarBaseRegIndex]);

    return kDoneLang;
}

static enum LangError SysCallNASM (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "SysCall started analyzing\n");

    size_t syscall_index = IndexSysCall (cur_pos->instruction.label);
    fprintf (output_file, "\tsub %s, %lu\t ; Shift RSP\n"
                          "call %s_syscall\t ; Calling function\n"
                          "\tadd %s, %lu\t ; Shift back RSP\n",
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->max_tmp_counter + kIR_SYS_CALL_ARRAY [syscall_index].NumberOfArguments) * kNASMQWordSize,
                          cur_pos->instruction.label,
                          kNASMRegisters [kNASMRSPRegIndex],
                          (cur_pos->max_tmp_counter + kIR_SYS_CALL_ARRAY [syscall_index].NumberOfArguments) * kNASMQWordSize);

    if (kIR_SYS_CALL_ARRAY [syscall_index].HaveRetVal)
    {
        fprintf (output_file, "\tmov qword [%s-%lu], %s\t ; Save return value to tmp\n",
                              kNASMRegisters [kNASMTmpBaseRegIndex],
                              cur_pos->instruction.res_index * kNASMQWordSize,
                              kNASMRegisters [kNASMRetValRegIndex]);
    }

    if (cur_pos->instruction.res_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = cur_pos->instruction.res_index;
    }

    return kDoneLang;
}

static size_t IndexSysCall (const char* const syscall_name)
{
    ASSERT(syscall_name != NULL, "Invalid argument syscall_name\n");

    for (size_t syscall_index = 0; syscall_index < kIR_SYS_CALL_NUMBER; syscall_index++)
    {
        if (strcmp (kIR_SYS_CALL_ARRAY [syscall_index].Name, syscall_name) == 0)
        {
            return syscall_index;
        }
    }
    return ULLONG_MAX;
}

static enum LangError GlobalVarsNumNASM  (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "GlobalVarsNum started analyzing\n");

    fprintf (output_file, "\n%s:\n"
                          "\tmov %s, %s\t ; Set label to variables counter register\n"
                          "\tmov %s, %s\t ; Set label to global variables counter register\n"
                          "\tmov %s, %s\n"
                          "\tsub %s, %lu\t ; Set value to tmp variables counter register\n"
                          "\tsub %s, %lu\t ; Set value to RSP\n\n",
                          kNASMStartLabelName,
                          kNASMRegisters [kNASMVarBaseRegIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMGlobalVarsStartIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex], cur_pos->instruction.value_1.number_size_t,
                          kNASMRegisters [kNASMRSPRegIndex], cur_pos->instruction.value_1.number_size_t);

    cur_pos->global_vars_cnt = cur_pos->instruction.value_1.number_size_t;

    return kDoneLang;
}
