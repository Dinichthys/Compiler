#include "translationNASM.h"

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

static enum LangError GenerateAsmNASM (const char* const buffer, FILE* const output_file);

static enum LangError PrintStartProgram (FILE* const output_file);

static enum LangError CallFuncNASM      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError FuncBodyNASM      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError CondJumpNASM      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError AssignNASM        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError AssignVarNASM     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt, const size_t* const cnt_func_args,
                                        size_t* const max_tmp_counter);
static enum LangError AssignTmpNASM     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt, size_t* const max_tmp_counter);
static enum LangError AssignArgNASM     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const max_tmp_counter);

static enum LangError OperationNASM     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError LabelNASM         (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError ReturnNASM        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError SysCallNASM       (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError GlobalVarsNumNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);

static void SkipNumber (const char* const input_buf, size_t* const offset);

typedef enum LangError (*TranslateNASMFunc_t) (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter);

static const TranslateNASMFunc_t kTranslationArray [kIR_KEY_WORD_NUMBER] =
{
    CallFuncNASM,
    FuncBodyNASM,
    CondJumpNASM,
    AssignNASM,
    OperationNASM,
    LabelNASM,
    ReturnNASM,
    SysCallNASM,
    GlobalVarsNumNASM
};

enum LangError GenerateAsmNASMFromIR (FILE* const ir_file, FILE* const output_file)
{
    ASSERT (ir_file     != NULL, "Invalid argument ir_file in function GenerateASMNASMFromIR\n");
    ASSERT (output_file != NULL, "Invalid argument output_file in function GenerateASMNASMFromIR\n");

    char* buffer = ReadFileToBuffer (ir_file);
    if (buffer == NULL)
    {
        return kCantReadDataBase;
    }

    enum LangError result = GenerateAsmNASM (buffer, output_file);

    FREE_AND_NULL (buffer);
    return result;
}

static enum LangError GenerateAsmNASM (const char* const buffer, FILE* const output_file)
{
    ASSERT (buffer      != NULL, "Invalid argument buffer\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    int can_read = 1;
    size_t read_letters = skip_space_symbols (buffer);

    enum LangError result = kDoneLang;
    bool in_function = false;
    char func_name [kNASMFuncMaxNameLenIR] = "";
    size_t cnt_func_args = 0;
    size_t global_vars_cnt = 0;
    size_t max_tmp_counter = 0;

    PrintStartProgram (output_file);

    while (can_read)
    {
        read_letters += skip_space_symbols (buffer + read_letters);
        char key_word [kNASMIRWordMaxLen] = "";

        if (*(buffer + read_letters) == kNASMIRCommentSymbol)
        {
            const char* end_comment = strchr (buffer + read_letters, '\n');
            if (end_comment == NULL)
            {
                return kDoneLang;
            }
            read_letters = end_comment - buffer;
            read_letters += skip_space_symbols (buffer + read_letters);
        }

        can_read = sscanf (buffer + read_letters, "%[^ ^\t^\n^(]", key_word);
        if ((can_read == 0) || (strcmp (key_word, "") == 0))
        {
            break;
        }

        LOG (kDebug, "Key word = \"%s\"\n", key_word);

        read_letters += strlen (key_word);
        read_letters += skip_space_symbols (buffer + read_letters);
        if (*(buffer + read_letters) != kNASMBracketOpen)
        {
            return kNoBracketsIR;
        }
        read_letters++;

        for (size_t index_kw = 0; index_kw <= kIR_KEY_WORD_NUMBER; index_kw++)
        {
            if (strcmp (key_word, kIR_KEY_WORD_ARRAY [index_kw]) == 0)
            {
                if (index_kw == (size_t) IR_FUNCTION_BODY_INDEX)
                {
                    in_function = true;
                    max_tmp_counter = 0;
                }
                result = kTranslationArray [index_kw] (buffer, &read_letters, output_file, in_function,
                                                       func_name, &global_vars_cnt, &cnt_func_args, &max_tmp_counter);
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
                          kNASMStartRamName, kNASMStartLabelName);

    for (size_t syscall_index = 0; syscall_index < kIR_SYS_CALL_NUMBER; syscall_index++)
    {
        fprintf (output_file, "extern %s_syscall\n", kIR_SYS_CALL_ARRAY [syscall_index].Name);
    }

    return kDoneLang;
}

static enum LangError CallFuncNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
    *read_letters += strlen (TMP_PREFIX);
    if (ret_val_index > *max_tmp_counter)
    {
        *max_tmp_counter = ret_val_index;
    }
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char call_func_name [kNASMFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^)^ ^\n^\t^\r]", call_func_name);
    *read_letters += strlen (call_func_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    LOG (kDebug, "Calling function is \"%s\"\n", call_func_name);

    size_t cnt_call_func_args = 0;
    if (strcmp (call_func_name, kNASMMainFuncName) != 0)
    {
        sscanf (strchr (strchr (call_func_name, '_') + 1, '_'), "_%lu", &cnt_call_func_args);
    }

    fprintf (output_file, "\tadd %s, %lu\t ; Shift tmp counter\n"
                          "call %s\t ; Calling function\n"
                          "\tsub %s, %lu\t ; Shift back tmp counter\n\n"
                          "\tmov qword [%s+%lu], %s\t ; Save return value to tmp var\n\n"
                          "\tadd %s, %lu\t ; Shift RSP to skip arguments\n",
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          (*max_tmp_counter + 1) * kNASMQWordSize,
                          call_func_name,
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          (*max_tmp_counter + 1) * kNASMQWordSize,
                          kNASMRegisters [kNASMTmpBaseRegIndex], ret_val_index * kNASMQWordSize,
                          kNASMRegisters [kNASMRetValRegIndex],
                          kNASMRegisters [kNASMRSPRegIndex],
                          cnt_call_func_args * kNASMQWordSize);

    return kDoneLang;
}

static enum LangError FuncBodyNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r]", func_name);
    *read_letters += strlen (func_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    sscanf (buffer + *read_letters, "%lu", cnt_func_args);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t cnt_loc_vars = 0;
    sscanf (buffer + *read_letters, "%lu", &cnt_loc_vars);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    LOG (kDebug, "Start function \"%s\" body\n", func_name);

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
                          func_name,
                          kNASMRegisters [kNASMVarBaseRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMRBPRegIndex],
                          kNASMRegisters [kNASMVarBaseRegIndex], kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex], cnt_loc_vars * kNASMQWordSize,
                          kNASMRegisters [kNASMUnusedRegisterIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMRSPRegIndex], (kNASMSavedRegistersNum) * kNASMQWordSize,
                          kNASMRegisters [kNASMRBPRegIndex], kNASMRegisters [kNASMRSPRegIndex],
                          kNASMRegisters [kNASMRSPRegIndex], kNASMRegisters [kNASMUnusedRegisterIndex]);
                          // В стеке лежит 3 сохранённых регистра и адрес возврата, а затем аргументы

    return kDoneLang;
}

static enum LangError CondJumpNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    char label_name [kNASMFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r]", label_name);
    *read_letters += strlen (label_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t cond_res_tmp_index = 0;
    bool cond_jump = (*(buffer + *read_letters) != kNASMTrueSymbol);
    if (cond_jump)
    {
        sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &cond_res_tmp_index);
        *read_letters += strlen (TMP_PREFIX);
        if (cond_res_tmp_index > *max_tmp_counter)
        {
            *max_tmp_counter = cond_res_tmp_index;
        }
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);
    }
    else
    {
        (*read_letters)++;
    }

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (cond_jump)
    {
        if (in_function)
        {
            fprintf (output_file, "\tmov %s, qword [%s+%lu]\n"
                                  "\ttest %s, %s\t ; Compare the result of the condition\n"
                                  "jne %s_%s\t ; Jump on label if the condition is true\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMTmpBaseRegIndex], cond_res_tmp_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  func_name, label_name);
        }
        else
        {
            fprintf (output_file, "\tmov %s, qword [%s+%lu]\n"
                                  "\ttest %s, %s\t ; Compare the result of the condition\n"
                                  "jne %s\t ; Jump on label if the condition is true\n",
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMTmpBaseRegIndex], cond_res_tmp_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  kNASMRegisters [kNASMUnusedRegisterIndex],
                                  label_name);
        }
    }
    else
    {
        if (in_function)
        {
            fprintf (output_file, "jmp %s_%s\t ; Jump on label\n", func_name, label_name);
        }
        else
        {
            fprintf (output_file, "jmp %s\t ; Jump on label\n", label_name);
        }
    }

    return kDoneLang;
}

static enum LangError AssignNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    char prefix_name [kNASMPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kNASMPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        return AssignVarNASM (buffer, read_letters, output_file, global_vars_cnt, cnt_func_args, max_tmp_counter);
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        return AssignTmpNASM (buffer, read_letters, output_file, global_vars_cnt, max_tmp_counter);
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        return AssignArgNASM (buffer, read_letters, output_file, max_tmp_counter);
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignVarNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    size_t* const global_vars_cnt, const size_t* const cnt_func_args,
                                    size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    long long var_index = 0;
    sscanf (buffer + *read_letters, "%lld", &var_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char prefix_name [kNASMPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kNASMPrefixLen - 1;

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        size_t src_tmp_index = 0;
        sscanf (buffer + *read_letters, "%lu", &src_tmp_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (src_tmp_index > *max_tmp_counter)
        {
            *max_tmp_counter = src_tmp_index;
        }

        if (*(buffer + *read_letters) != kNASMBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kNASMRegisters [kNASMTmpBaseRegIndex], src_tmp_index * kNASMQWordSize,
                                  kNASMRegisters [kNASMVarBaseRegIndex], ((size_t) var_index) * kNASMQWordSize);
        }
        else
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kNASMRegisters [kNASMTmpBaseRegIndex], src_tmp_index * kNASMQWordSize,
                                  kNASMStartRamName,
                                  (*global_vars_cnt + 1 - (size_t) (-var_index)) * kNASMQWordSize);
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        size_t arg_index = 0;
        sscanf (buffer + *read_letters, "%lu", &arg_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kNASMBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tadd %s, %lu\t ; Save value temporary to RBP\n"
                                  "\tpush qword [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  (*cnt_func_args - arg_index) * kNASMQWordSize,
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  kNASMRegisters [kNASMVarBaseRegIndex], ((size_t) var_index) * kNASMQWordSize,
                                  kNASMRegisters [kNASMRBPRegIndex]);
        }
        else
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tadd %s, %lu\t ; Save value temporary to RBP\n"
                                  "\tpush qword [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop qword [%s+%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  (*cnt_func_args - arg_index) * kNASMQWordSize,
                                  kNASMRegisters [kNASMRBPRegIndex],
                                  kNASMStartRamName,
                                  (*global_vars_cnt + 1 - (size_t) (-var_index)) * kNASMQWordSize,
                                  kNASMRegisters [kNASMRBPRegIndex]);
        }

        return kDoneLang;
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignTmpNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    size_t* const global_vars_cnt, size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t result_tmp_index = 0;
    sscanf (buffer + *read_letters, "%lu", &result_tmp_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (result_tmp_index > *max_tmp_counter)
    {
        *max_tmp_counter = result_tmp_index;
    }

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (isdigit (*(buffer + *read_letters)))
    {
        float number = 0;
        sscanf (buffer + *read_letters, "%g", &number);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kNASMBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        fprintf (output_file, "\tpush 0x%08x\n"
                              "\tpop qword [%s+%lu]\t ; Push the number %g to tmp through the stack\n",
                               *((unsigned int*) (&number)),
                              kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize,
                              number);

        return kDoneLang;
    }

    char prefix_name [kNASMPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kNASMPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        long long var_index = 0;
        sscanf (buffer + *read_letters, "%lld", &var_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kNASMBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kNASMRegisters [kNASMVarBaseRegIndex], ((size_t) var_index) * kNASMQWordSize,
                                  kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize);
        }
        else
        {
            fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop qword [%s+%lu]\n\n",
                                  kNASMStartRamName,
                                  (*global_vars_cnt + 1 - (size_t) (-var_index)) * kNASMQWordSize,
                                  kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize);
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        size_t src_tmp_index = 0;
        sscanf (buffer + *read_letters, "%lu", &src_tmp_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (src_tmp_index > *max_tmp_counter)
        {
            *max_tmp_counter = src_tmp_index;
        }

        if (*(buffer + *read_letters) != kNASMBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp to tmp through the stack\n"
                              "\tpop qword [%s+%lu]\n\n",
                              kNASMRegisters [kNASMTmpBaseRegIndex], src_tmp_index * kNASMQWordSize,
                              kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize);

        return kDoneLang;
    }

    return kInvalidAssigning;
}

static enum LangError AssignArgNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t result_arg_index = 0;
    sscanf (buffer + *read_letters, "%lu", &result_arg_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t src_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &src_tmp_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (src_tmp_index > *max_tmp_counter)
    {
        *max_tmp_counter = src_tmp_index;
    }

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\tpush qword [%s+%lu]\t ; Push tmp %lu to argument %lu through the stack\n",
                          kNASMRegisters [kNASMTmpBaseRegIndex], src_tmp_index * kNASMQWordSize,
                          src_tmp_index, result_arg_index);

    return kDoneLang;
}

static enum LangError OperationNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t result_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &result_tmp_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (result_tmp_index > *max_tmp_counter)
    {
        *max_tmp_counter = result_tmp_index;
    }

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    unsigned int operation_enum = 0;
    sscanf (buffer + *read_letters, "%u", &operation_enum);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t first_operand_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &first_operand_tmp_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (first_operand_tmp_index > *max_tmp_counter)
    {
        *max_tmp_counter = first_operand_tmp_index;
    }

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t second_operand_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &second_operand_tmp_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (second_operand_tmp_index > *max_tmp_counter)
    {
        *max_tmp_counter = second_operand_tmp_index;
    }

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (operation_enum < kNASMIRArithmOperationsNum)
    {
        fprintf (output_file, "\tmovq %s, qword [%s+%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s+%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\n"
                              "\tmovq qword [%s+%lu], %s\t ; Move the result to the tmp var\n\n",
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], first_operand_tmp_index * kNASMQWordSize,
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], second_operand_tmp_index * kNASMQWordSize,
                              kInstructionsArithmNASM [operation_enum - 1], // Енамы нумеруются с 1, а элементы массива с 0
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize, kNASMRegisters [kNASMFirstOpRegIndex]);
    }
    else
    {
        fprintf (output_file, "\tmovq %s, qword [%s+%lu]\t ; Push first operand\n"
                              "\tmovq %s, qword [%s+%lu]\t ; Push second operand\n"
                              "\t%s %s, %s\t ; Comparison\n"
                              "\tmovq %s, %s\t ; Move the result of comparison to the unused reg\n"
                              "\tshr %s, 31\t ; Make the result 1 or 0\n"
                              "\tmov qword [%s+%lu], %s\t ; Move the result to the tmp var\n\n",
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], first_operand_tmp_index * kNASMQWordSize,
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], second_operand_tmp_index * kNASMQWordSize,
                              kInstructionsLogicNASM [operation_enum - kNASMIRArithmOperationsNum - 1],
                              kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMSecondOpRegIndex],
                              kNASMRegisters [kNASMUnusedRegisterIndex], kNASMRegisters [kNASMFirstOpRegIndex],
                              kNASMRegisters [kNASMUnusedRegisterIndex],
                              kNASMRegisters [kNASMTmpBaseRegIndex], result_tmp_index * kNASMQWordSize, kNASMRegisters [kNASMUnusedRegisterIndex]);
    }

    return kDoneLang;
}

static enum LangError LabelNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                __attribute_maybe_unused__ bool in_function,
                                __attribute_maybe_unused__ char* const func_name,
                                __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                __attribute_maybe_unused__ size_t* const cnt_func_args,
                                __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    char label_name [kNASMFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r^)]", label_name);
    *read_letters += strlen (label_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    LOG (kDebug, "Label \"%s\" was analyzed\n", label_name);

    if (in_function)
    {
        fprintf (output_file, "%s_%s:\t ; Label in function\n", func_name, label_name);
    }
    else
    {
        fprintf (output_file, "%s:\t ; Label in global\n", label_name);
    }

    return kDoneLang;
}

static enum LangError ReturnNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                 __attribute_maybe_unused__ bool in_function,
                                 __attribute_maybe_unused__ char* const func_name,
                                 __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                 __attribute_maybe_unused__ size_t* const cnt_func_args,
                                 __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (ret_val_index > *max_tmp_counter)
    {
        *max_tmp_counter = ret_val_index;
    }

    const char* end_args = strchr (buffer + *read_letters, kNASMBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    *read_letters = end_args - buffer;
    (*read_letters)++;

    fprintf (output_file, "\n\tpush qword [%s+%lu]\n"
                          "\tpop %s\t ; Save return value to the rax\n"
                          "\tpop %s\t ; Get RBP value back\n"
                          "\tpop %s\t ; Get tmp register value back\n"
                          "\tpop %s\t ; Get var register value back\n"
                          "ret\n",
                          kNASMRegisters [kNASMTmpBaseRegIndex], ret_val_index * kNASMQWordSize,
                          kNASMRegisters [kNASMRetValRegIndex],
                          kNASMRegisters [kNASMRBPRegIndex],
                          kNASMRegisters [kNASMTmpBaseRegIndex],
                          kNASMRegisters [kNASMVarBaseRegIndex]);

    return kDoneLang;
}

static enum LangError SysCallNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
    *read_letters += strlen (TMP_PREFIX);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (ret_val_index > *max_tmp_counter)
    {
        *max_tmp_counter = ret_val_index;
    }

    if (*(buffer + *read_letters) != kNASMSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char sys_func_name [kNASMFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^)^ ^\n^\t^\r^,]", sys_func_name);
    *read_letters += strlen (sys_func_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    const char* end_args = strchr (buffer + *read_letters, kNASMBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    *read_letters = end_args - buffer;
    (*read_letters)++;

    fprintf (output_file, "call %s_syscall\t ; System call\n", sys_func_name);

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_OUT_INDEX].Name) != 0)
    {
        fprintf (output_file, "\tmov qword [%s+%lu], %s\t ; Save return value to tmp\n",
                              kNASMRegisters [kNASMTmpBaseRegIndex], ret_val_index * kNASMQWordSize,
                              kNASMRegisters [kNASMRetValRegIndex]);
    }

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_IN_INDEX].Name) == 0)
    {
        fprintf (output_file, "\tpop %s\t ; Pop argument out of stack\n\n", kNASMRegisters [kNASMUnusedRegisterIndex]);
    }

    return kDoneLang;
}

static enum LangError GlobalVarsNumNASM (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                       __attribute_maybe_unused__ bool in_function,
                                       __attribute_maybe_unused__ char* const func_name,
                                       __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                       __attribute_maybe_unused__ size_t* const cnt_func_args,
                                       __attribute_maybe_unused__ size_t* const max_tmp_counter)
{
    ASSERT (buffer          != NULL, "Invalid argument buffer\n");
    ASSERT (output_file     != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters    != NULL, "Invalid argument read_letters\n");
    ASSERT (max_tmp_counter != NULL, "Invalid argument max_tmp_counter\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t cnt_glob_vars = 0;
    sscanf (buffer + *read_letters, "%lu", &cnt_glob_vars);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kNASMBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\n%s:\n"
                          "\tmov %s, %s\t ; Set label to global variables counter register\n"
                          "\tmov %s, %s\n"
                          "\tadd %s, %lu\t ; Set value to tmp variables counter register\n\n",
                          kNASMStartLabelName,
                          kNASMRegisters [kNASMVarBaseRegIndex], kNASMStartRamName,
                          kNASMRegisters [kNASMTmpBaseRegIndex], kNASMStartRamName,
                          kNASMRegisters [kNASMTmpBaseRegIndex], cnt_glob_vars);

    *global_vars_cnt = cnt_glob_vars;

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
