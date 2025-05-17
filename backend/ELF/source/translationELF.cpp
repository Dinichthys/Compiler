#include "translationELF.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "language.h"

#include "key_words.h"
#include "dsl_write.h"

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

#define CHECK_RESULT            \
    if (result != kDoneLang)    \
    {                           \
        return result;          \
    }

#define CALL_FUNC(func_name)                                                                                    \
    fprintf (output_file, "call %s:\t ; Call function\n", func_name, cnt_args);

typedef struct current_position
{
    const char* buffer;
    size_t      read_letters;
    char        func_name [kELFFuncMaxNameLenIR];
    size_t      global_vars_cnt;
    size_t      cnt_func_args;
    size_t      max_tmp_counter;
} current_position_t;

static enum LangError GenerateAsmELF (const char* const buffer, FILE* const output_file);

static enum LangError PrintStartProgram (FILE* const output_file);

static enum LangError CallFuncELF      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError FuncBodyELF      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError CondJumpELF      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignELF        (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError AssignVarELF     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignTmpELF     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignArgELF     (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError OperationELF     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError LabelELF         (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError ReturnELF        (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError SysCallELF       (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError GlobalVarsNumELF (current_position_t* const cur_pos, FILE* const output_file);

static void SkipNumber (const char* const input_buf, size_t* const offset);

typedef enum LangError (*TranslateELFFunc_t) (current_position_t* const cur_pos, FILE* const output_file);

static const TranslateELFFunc_t kTranslationArray [kIR_KEY_WORD_NUMBER] =
{
    CallFuncELF,
    FuncBodyELF,
    CondJumpELF,
    AssignELF,
    OperationELF,
    LabelELF,
    ReturnELF,
    SysCallELF,
    GlobalVarsNumELF
};

enum LangError GenerateAsmELFFromIR (FILE* const ir_file, FILE* const output_file)
{
    ASSERT (ir_file     != NULL, "Invalid argument ir_file in function GenerateASMELFFromIR\n");
    ASSERT (output_file != NULL, "Invalid argument output_file in function GenerateASMELFFromIR\n");

    char* buffer = ReadFileToBuffer (ir_file);
    if (buffer == NULL)
    {
        return kCantReadDataBase;
    }

    enum LangError result = GenerateAsmELF (buffer, output_file);

    FREE_AND_NULL (buffer);
    return result;
}

static enum LangError GenerateAsmELF (const char* const buffer, FILE* const output_file)
{
    ASSERT (buffer      != NULL, "Invalid argument buffer\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    int can_read = 1;

    enum LangError result = kDoneLang;

    current_position_t cur_pos =
    {
        .buffer = buffer,
        .read_letters = 0,
        .func_name = "global",
        .global_vars_cnt = 0,
        .cnt_func_args = 0,
        .max_tmp_counter = 0,
    };

    cur_pos.read_letters = skip_space_symbols (buffer);

    PrintStartProgram (output_file);

    while (can_read)
    {
        cur_pos.read_letters += skip_space_symbols (buffer + cur_pos.read_letters);
        char key_word [kELFIRWordMaxLen] = "";

        if (*(buffer + cur_pos.read_letters) == kELFIRCommentSymbol)
        {
            const char* end_comment = strchr (buffer + cur_pos.read_letters, '\n');
            if (end_comment == NULL)
            {
                return kDoneLang;
            }
            cur_pos.read_letters = end_comment - buffer;
            cur_pos.read_letters += skip_space_symbols (buffer + cur_pos.read_letters);
        }

        can_read = sscanf (buffer + cur_pos.read_letters, "%[^ ^\t^\n^(]", key_word);
        if ((can_read == 0) || (strcmp (key_word, "") == 0))
        {
            break;
        }

        LOG (kDebug, "Key word = \"%s\"\n", key_word);

        cur_pos.read_letters += strlen (key_word);
        cur_pos.read_letters += skip_space_symbols (buffer + cur_pos.read_letters);
        if (*(buffer + cur_pos.read_letters) != kELFBracketOpen)
        {
            return kNoBracketsIR;
        }
        cur_pos.read_letters++;

        for (size_t index_kw = 0; index_kw <= kIR_KEY_WORD_NUMBER; index_kw++)
        {
            if (strcmp (key_word, kIR_KEY_WORD_ARRAY [index_kw]) == 0)
            {
                if (index_kw == (size_t) IR_FUNCTION_BODY_INDEX)
                {
                    cur_pos.max_tmp_counter = 0;
                }
                result = kTranslationArray [index_kw] (&cur_pos, output_file);
                CHECK_RESULT;
                break;
            }
        }
    }

    return kDoneLang;
}

static enum LangError PrintStartProgram (FILE* const output_file)
{
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    fprintf (output_file, "section .data\n\n"
                          "\t%s dq 0\n\n"
                          "section .text\n\n"
                          "global %s\n\n",
                          kELFStartRamName, kELFStartLabelName);

    for (size_t syscall_index = 0; syscall_index < kIR_SYS_CALL_NUMBER; syscall_index++)
    {
        fprintf (output_file, "extern %s_syscall\n", kIR_SYS_CALL_ARRAY [syscall_index].Name);
    }

    return kDoneLang;
}

static enum LangError CallFuncELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t ret_val_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &ret_val_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    if (ret_val_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = ret_val_index;
    }
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char call_func_name [kELFFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^)^ ^\n^\t^\r]", call_func_name);
    cur_pos->read_letters += strlen (call_func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    LOG (kDebug, "Calling function is \"%s\"\n", call_func_name);

    size_t cnt_call_func_args = 0;
    if (strcmp (call_func_name, kELFMainFuncName) != 0)
    {
        sscanf (strchr (strchr (call_func_name, '_') + 1, '_'), "_%lu", &cnt_call_func_args);
    }

    fprintf (output_file, "\tadd %s, %lu\t ; Shift tmp counter\n"
                          "call %s\t ; Calling function\n"
                          "\tsub %s, %lu\t ; Shift back tmp counter\n\n"
                          "\tmov qword [%s+%lu], %s\t ; Save return value to tmp var\n\n"
                          "\tadd %s, %lu\t ; Shift RSP to skip arguments\n",
                          kELFRegisters [kELFTmpBaseRegIndex],
                          (cur_pos->max_tmp_counter + 1) * kELFQWordSize,
                          call_func_name,
                          kELFRegisters [kELFTmpBaseRegIndex],
                          (cur_pos->max_tmp_counter + 1) * kELFQWordSize,
                          kELFRegisters [kELFTmpBaseRegIndex], ret_val_index * kELFQWordSize,
                          kELFRegisters [kELFRetValRegIndex],
                          kELFRegisters [kELFRSPRegIndex],
                          cnt_call_func_args * kELFQWordSize);

    return kDoneLang;
}

static enum LangError FuncBodyELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r]", cur_pos->func_name);
    cur_pos->read_letters += strlen (cur_pos->func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &(cur_pos->cnt_func_args));
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t cnt_loc_vars = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &cnt_loc_vars);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    LOG (kDebug, "Start function \"%s\" body\n", cur_pos->func_name);

    fprintf (output_file, "\n%s:\t ; Function body\n"
                          "\tpush %s\t ; Save variables counter register value\n"
                          "\tpush %s\t ; Save tmp variables counter register value\n"
                          "\tpush %s\t ; Save RBP\n\n"
                          "\tmov %s, %s\t ; Move tmp var counter reg value to the var counter reg\n\n"
                          "\tadd %s, %lu\t ; Make a stack frame for local variables\n\n"
                          "\tmov %s, %s\t ; Save old RSP value\n"
                          "\tadd %s, %lu\n"
                          "\tmov %s, %s\t ; Make RBP point on the last arg\n"
                          "\tmov %s, %s\t ; Save old RSP value\n",
                          cur_pos->func_name,
                          kELFRegisters [kELFVarBaseRegIndex],
                          kELFRegisters [kELFTmpBaseRegIndex],
                          kELFRegisters [kELFRBPRegIndex],
                          kELFRegisters [kELFVarBaseRegIndex], kELFRegisters [kELFTmpBaseRegIndex],
                          kELFRegisters [kELFTmpBaseRegIndex], cnt_loc_vars * kELFQWordSize,
                          kELFRegisters [kELFUnusedRegisterIndex], kELFRegisters [kELFRSPRegIndex],
                          kELFRegisters [kELFRSPRegIndex], (kELFSavedRegistersNum) * kELFQWordSize,
                          kELFRegisters [kELFRBPRegIndex], kELFRegisters [kELFRSPRegIndex],
                          kELFRegisters [kELFRSPRegIndex], kELFRegisters [kELFUnusedRegisterIndex]);
                          // В стеке лежит 3 сохранённых регистра и адрес возврата, а затем аргументы

    return kDoneLang;
}

static enum LangError CondJumpELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char label_name [kELFFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r]", label_name);
    cur_pos->read_letters += strlen (label_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t cond_res_tmp_index = 0;
    bool cond_jump = (*(cur_pos->buffer + cur_pos->read_letters) != kELFTrueSymbol);
    if (cond_jump)
    {
        sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &cond_res_tmp_index);
        cur_pos->read_letters += strlen (TMP_PREFIX);
        if (cond_res_tmp_index > cur_pos->max_tmp_counter)
        {
            cur_pos->max_tmp_counter = cond_res_tmp_index;
        }
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);
    }
    else
    {
        cur_pos->read_letters++;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (cond_jump)
    {
        fprintf (output_file, "\tmov %s, qword [%s+%lu]\n"
                              "\ttest %s, %s\t ; Compare the result of the condition\n"
                              "jne %s_%s\t ; Jump on label if the condition is true\n",
                              kELFRegisters [kELFUnusedRegisterIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], cond_res_tmp_index * kELFQWordSize,
                              kELFRegisters [kELFUnusedRegisterIndex],
                              kELFRegisters [kELFUnusedRegisterIndex],
                              cur_pos->func_name, label_name);
    }
    else
    {
        fprintf (output_file, "jmp %s_%s\t ; Jump on label\n", cur_pos->func_name, label_name);
    }

    return kDoneLang;
}

static enum LangError AssignELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char prefix_name [kELFPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kELFPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        return AssignVarELF (cur_pos, output_file);
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        return AssignTmpELF (cur_pos, output_file);
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        return AssignArgELF (cur_pos, output_file);
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignVarELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    long long var_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lld", &var_index);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char prefix_name [kELFPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kELFPrefixLen - 1;

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        size_t src_tmp_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &src_tmp_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (src_tmp_index > cur_pos->max_tmp_counter)
        {
            cur_pos->max_tmp_counter = src_tmp_index;
        }

        if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kELFRegisters [kELFTmpBaseRegIndex], src_tmp_index * kELFQWordSize,
                                  kELFRegisters [kELFVarBaseRegIndex], ((size_t) var_index) * kELFQWordSize);
        }
        else
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kELFRegisters [kELFTmpBaseRegIndex], src_tmp_index * kELFQWordSize,
                                  kELFStartRamName,
                                  (cur_pos->global_vars_cnt + 1 - (size_t) (-var_index)) * kELFQWordSize);
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        size_t arg_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &arg_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tadd %s, %lu\t ; Save value temporary to RBP\n"
                                  "\tpush qword [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  kELFRegisters [kELFRBPRegIndex],
                                  kELFRegisters [kELFRBPRegIndex],
                                  (cur_pos->cnt_func_args - arg_index) * kELFQWordSize,
                                  kELFRegisters [kELFRBPRegIndex],
                                  kELFRegisters [kELFVarBaseRegIndex], ((size_t) var_index) * kELFQWordSize,
                                  kELFRegisters [kELFRBPRegIndex]);
        }
        else
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tadd %s, %lu\t ; Save value temporary to RBP\n"
                                  "\tpush qword [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  kELFRegisters [kELFRBPRegIndex],
                                  kELFRegisters [kELFRBPRegIndex],
                                  (cur_pos->cnt_func_args - arg_index) * kELFQWordSize,
                                  kELFRegisters [kELFRBPRegIndex],
                                  kELFStartRamName,
                                  (cur_pos->global_vars_cnt + 1 - (size_t) (-var_index)) * kELFQWordSize,
                                  kELFRegisters [kELFRBPRegIndex]);
        }

        return kDoneLang;
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignTmpELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t result_tmp_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &result_tmp_index);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (result_tmp_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = result_tmp_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (isdigit (*(cur_pos->buffer + cur_pos->read_letters)))
    {
        float number = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%g", &number);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        fprintf (output_file, "\tpush 0x%08x\n"
                              "\tpop qword [%s+%lu]\t ; Push the number %g to tmp through the stack\n",
                               *((unsigned int*) (&number)),
                              kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize,
                              number);

        return kDoneLang;
    }

    char prefix_name [kELFPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kELFPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        long long var_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lld", &var_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kELFRegisters [kELFVarBaseRegIndex], ((size_t) var_index) * kELFQWordSize,
                                  kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize);
        }
        else
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kELFStartRamName,
                                  (cur_pos->global_vars_cnt + 1 - (size_t) (-var_index)) * kELFQWordSize,
                                  kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize);
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        size_t src_tmp_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &src_tmp_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (src_tmp_index > cur_pos->max_tmp_counter)
        {
            cur_pos->max_tmp_counter = src_tmp_index;
        }

        if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to tmp through the stack\n"
                              "\tpop qword [%s+%lu]\n\n",
                              kELFRegisters [kELFTmpBaseRegIndex], src_tmp_index * kELFQWordSize,
                              kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize);

        return kDoneLang;
    }

    return kInvalidAssigning;
}

static enum LangError AssignArgELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t result_arg_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &result_arg_index);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t src_tmp_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &src_tmp_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (src_tmp_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = src_tmp_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp %lu to argument %lu through the stack\n",
                          kELFRegisters [kELFTmpBaseRegIndex], src_tmp_index * kELFQWordSize,
                          src_tmp_index, result_arg_index);

    return kDoneLang;
}

static enum LangError OperationELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t result_tmp_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &result_tmp_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (result_tmp_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = result_tmp_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    unsigned int operation_enum = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%u", &operation_enum);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t first_operand_tmp_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &first_operand_tmp_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (first_operand_tmp_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = first_operand_tmp_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t second_operand_tmp_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &second_operand_tmp_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (second_operand_tmp_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = second_operand_tmp_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (operation_enum < kELFIRArithmOperationsNum)
    {
        fprintf (output_file, "\tmovq %s, qword [%s+%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s+%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\n"
                              "\tmovq qword [%s+%lu], %s\t ; Move the result to the tmp var\n\n",
                              kELFRegisters [kELFFirstOpRegIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], first_operand_tmp_index * kELFQWordSize,
                              kELFRegisters [kELFSecondOpRegIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], second_operand_tmp_index * kELFQWordSize,
                              kInstructionsArithmELF [operation_enum - 1], // Енамы нумеруются с 1, а элементы массива с 0
                              kELFRegisters [kELFFirstOpRegIndex],
                              kELFRegisters [kELFSecondOpRegIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize, kELFRegisters [kELFFirstOpRegIndex]);
    }
    else
    {
        fprintf (output_file, "\tmovq %s, qword [%s+%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s+%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\t ; Comparison\n"
                              "\tmovq %s, %s\t ; Move the result of comparison to the unused reg\n"
                              "\tshr %s, 31\t ; Make the result 1 or 0\n"
                              "\tmov qword [%s+%lu], %s\t ; Move the result to the tmp var\n\n",
                              kELFRegisters [kELFFirstOpRegIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], first_operand_tmp_index * kELFQWordSize,
                              kELFRegisters [kELFSecondOpRegIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], second_operand_tmp_index * kELFQWordSize,
                              kInstructionsLogicELF [operation_enum - kELFIRArithmOperationsNum - 1],
                              kELFRegisters [kELFFirstOpRegIndex],
                              kELFRegisters [kELFSecondOpRegIndex],
                              kELFRegisters [kELFUnusedRegisterIndex], kELFRegisters [kELFFirstOpRegIndex],
                              kELFRegisters [kELFUnusedRegisterIndex],
                              kELFRegisters [kELFTmpBaseRegIndex], result_tmp_index * kELFQWordSize, kELFRegisters [kELFUnusedRegisterIndex]);
    }

    return kDoneLang;
}

static enum LangError LabelELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char label_name [kELFFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r^)]", label_name);
    cur_pos->read_letters += strlen (label_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    LOG (kDebug, "Label \"%s\" was analyzed\n", label_name);

    fprintf (output_file, "%s_%s:\t ; Label in function\n", cur_pos->func_name, label_name);

    return kDoneLang;
}

static enum LangError ReturnELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t ret_val_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &ret_val_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (ret_val_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = ret_val_index;
    }

    const char* end_args = strchr (cur_pos->buffer + cur_pos->read_letters, kELFBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters = end_args - cur_pos->buffer;
    cur_pos->read_letters++;

    fprintf (output_file, "\n\tpush qword [%s+%lu]\n"
                          "\tpop %s\t ; Save return value to the rax\n"
                          "\tpop %s\t ; Get RBP value back\n"
                          "\tpop %s\t ; Get tmp register value back\n"
                          "\tpop %s\t ; Get var register value back\n"
                          "ret\n",
                          kELFRegisters [kELFTmpBaseRegIndex], ret_val_index * kELFQWordSize,
                          kELFRegisters [kELFRetValRegIndex],
                          kELFRegisters [kELFRBPRegIndex],
                          kELFRegisters [kELFTmpBaseRegIndex],
                          kELFRegisters [kELFVarBaseRegIndex]);

    return kDoneLang;
}

static enum LangError SysCallELF (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t ret_val_index = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, TMP_PREFIX "%lu", &ret_val_index);
    cur_pos->read_letters += strlen (TMP_PREFIX);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (ret_val_index > cur_pos->max_tmp_counter)
    {
        cur_pos->max_tmp_counter = ret_val_index;
    }

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char sys_func_name [kELFFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^)^ ^\n^\t^\r^,]", sys_func_name);
    cur_pos->read_letters += strlen (sys_func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    const char* end_args = strchr (cur_pos->buffer + cur_pos->read_letters, kELFBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters = end_args - cur_pos->buffer;
    cur_pos->read_letters++;

    fprintf (output_file, "call %s_syscall\t ; System call\n", sys_func_name);

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_OUT_INDEX].Name) != 0)
    {
        fprintf (output_file, "\tmov qword [%s+%lu], %s\t ; Save return value to tmp\n",
                              kELFRegisters [kELFTmpBaseRegIndex], ret_val_index * kELFQWordSize,
                              kELFRegisters [kELFRetValRegIndex]);
    }

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_IN_INDEX].Name) == 0)
    {
        fprintf (output_file, "\tpop %s\t ; Pop argument out of stack\n\n", kELFRegisters [kELFUnusedRegisterIndex]);
    }

    return kDoneLang;
}

static enum LangError GlobalVarsNumELF  (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    size_t cnt_glob_vars = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &cnt_glob_vars);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kELFBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    fprintf (output_file, "\n%s:\n"
                          "\tmov %s, %s\t ; Set label to global variables counter register\n"
                          "\tmov %s, %s\n"
                          "\tadd %s, %lu\t ; Set value to tmp variables counter register\n\n",
                          kELFStartLabelName,
                          kELFRegisters [kELFVarBaseRegIndex], kELFStartRamName,
                          kELFRegisters [kELFTmpBaseRegIndex], kELFStartRamName,
                          kELFRegisters [kELFTmpBaseRegIndex], cnt_glob_vars);

    cur_pos->global_vars_cnt = cnt_glob_vars;

    return kDoneLang;
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
