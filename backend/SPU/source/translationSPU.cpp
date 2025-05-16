#include "translationSPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "language.h"
#include "program.h"

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

static enum LangError GenerateAsmSPU (const char* const buffer, FILE* const output_file);

static enum LangError CallFuncSPU      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError FuncBodySPU      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError CondJumpSPU      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError AssignSPU        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError AssignVarSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt, const size_t* const cnt_func_args,
                                        size_t* const max_tmp_counter);
static enum LangError AssignTmpSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt, size_t* const max_tmp_counter);
static enum LangError AssignArgSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const max_tmp_counter);

static enum LangError OperationSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError LabelSPU         (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError ReturnSPU        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError SysCallSPU       (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);
static enum LangError GlobalVarsNumSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args,
                                        __attribute_maybe_unused__ size_t* const max_tmp_counter);

static void SkipNumber (const char* const input_buf, size_t* const offset);

typedef enum LangError (*TranslateSpuFunc_t) (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args,
                                    __attribute_maybe_unused__ size_t* const max_tmp_counter);

static const TranslateSpuFunc_t kTranslationArray [kIR_KEY_WORD_NUMBER] =
{
    CallFuncSPU,
    FuncBodySPU,
    CondJumpSPU,
    AssignSPU,
    OperationSPU,
    LabelSPU,
    ReturnSPU,
    SysCallSPU,
    GlobalVarsNumSPU
};

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file)
{
    ASSERT (ir_file     != NULL, "Invalid argument ir_file in function GenerateASMSPUFromIR\n");
    ASSERT (output_file != NULL, "Invalid argument output_file in function GenerateASMSPUFromIR\n");

    char* buffer = ReadFileToBuffer (ir_file);
    if (buffer == NULL)
    {
        return kCantReadDataBase;
    }

    enum LangError result = GenerateAsmSPU (buffer, output_file);

    FREE_AND_NULL (buffer);
    return result;
}

static enum LangError GenerateAsmSPU (const char* const buffer, FILE* const output_file)
{
    ASSERT (buffer      != NULL, "Invalid argument buffer\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    int can_read = 1;
    size_t read_letters = skip_space_symbols (buffer);

    enum LangError result = kDoneLang;
    bool in_function = false;
    char func_name [kFuncMaxNameLenIR] = "";
    size_t cnt_func_args = 0;
    size_t global_vars_cnt = 0;
    size_t max_tmp_counter = 0;

    while (can_read)
    {
        read_letters += skip_space_symbols (buffer + read_letters);
        char key_word [kIRWordMaxLen] = "";

        if (*(buffer + read_letters) == kIRCommentSymbol)
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
        if (*(buffer + read_letters) != kBracketOpen)
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

static enum LangError CallFuncSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char call_func_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^)^ ^\n^\t^\r]", call_func_name);
    *read_letters += strlen (call_func_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    LOG (kDebug, "Calling function is \"%s\"\n", call_func_name);

    size_t cnt_call_func_args = 0;
    if (strcmp (call_func_name, kMainFuncName) != 0)
    {
        sscanf (strchr (strchr (call_func_name, '_') + 1, '_'), "_%lu", &cnt_call_func_args);
    }

    fprintf (output_file, "\tpush %lu\n"
                          "\tpush %s\n"
                          "\tadd\n"
                          "\tpop %s\t ; Shift tmp counter\n"
                          "call %s:\t ; Calling function\n"
                          "\tpush %lu\n"
                          "\tpush %s\n"
                          "\tsub\n"
                          "\tpop %s\t ; Shift back tmp counter\n\n"
                          "\tpush %s\t ; Push return value to the stack\n"
                          "\tpop [%s+%lu]\t ; Save return value to tmp var\n\n"
                          "\tpush %lu\n"
                          "\tpush %s\n"
                          "\tsub\n"
                          "\tpop %s\t ; Shift RSP to skip arguments\n",
                          *max_tmp_counter + 1,
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          call_func_name,
                          *max_tmp_counter + 1,
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRetValRegIndex],
                          REGISTERS [kTmpBaseRegIndex], ret_val_index,
                          cnt_call_func_args + 1,
                          REGISTERS [kRSPRegIndex],
                          REGISTERS [kRSPRegIndex]);

    return kDoneLang;
}

static enum LangError FuncBodySPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    sscanf (buffer + *read_letters, "%lu", cnt_func_args);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t cnt_loc_vars = 0;
    sscanf (buffer + *read_letters, "%lu", &cnt_loc_vars);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kBracketClose)
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
                          "\tpush %s\n"
                          "\tpop %s\t ; Move tmp var counter reg value to the var counter reg\n\n"
                          "\tpush %s\n"
                          "\tpush %lu\n"
                          "\tadd\n"
                          "\tpop %s\t ; Make a stack frame for local variables\n\n"
                          "\tpush 4\n"
                          "\tpush %s\n"
                          "\tsub\t ; Make RBP point on the last arg\n"
                          "\tpop %s\t ; Move RSP to RBP\n\n",
                          func_name,
                          REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRBPRegIndex],
                          REGISTERS [kTmpBaseRegIndex], REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex], cnt_loc_vars, REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRSPRegIndex], REGISTERS [kRBPRegIndex]);

    return kDoneLang;
}

static enum LangError CondJumpSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    char label_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r]", label_name);
    *read_letters += strlen (label_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t cond_res_tmp_index = 0;
    bool cond_jump = (*(buffer + *read_letters) != kTrueSymbol);
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

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (cond_jump)
    {
        if (in_function)
        {
            fprintf (output_file, "\tpush [%s+%lu]\t ; Push the result of the condition to the stack\n"
                                  "\tpush 1\t ; Push true\n\n"
                                  "je %s_%s:\t ; Jump on label if the condition is true\n",
                                  REGISTERS [kTmpBaseRegIndex], cond_res_tmp_index,
                                  func_name, label_name);
        }
        else
        {
            fprintf (output_file, "\tpush [%s+%lu]\t ; Push the result of the condition to the stack\n"
                                  "\tpush 1\t ; Push true\n\n"
                                  "je %s:\t ; Jump on label if the condition is true\n",
                                  REGISTERS [kTmpBaseRegIndex], cond_res_tmp_index,
                                  label_name);
        }
    }
    else
    {
        if (in_function)
        {
            fprintf (output_file, "jmp %s_%s:\t ; Jump on label\n", func_name, label_name);
        }
        else
        {
            fprintf (output_file, "jmp %s:\t ; Jump on label\n", label_name);
        }
    }

    return kDoneLang;
}

static enum LangError AssignSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    char prefix_name [kPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        return AssignVarSPU (buffer, read_letters, output_file, global_vars_cnt, cnt_func_args, max_tmp_counter);
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        return AssignTmpSPU (buffer, read_letters, output_file, global_vars_cnt, max_tmp_counter);
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        return AssignArgSPU (buffer, read_letters, output_file, max_tmp_counter);
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignVarSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char prefix_name [kPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kPrefixLen - 1;

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

        if (*(buffer + *read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop [%s+%lu]\n\n",
                                  REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                                  REGISTERS [kVarBaseRegIndex], (size_t) var_index);
        }
        else
        {
            fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp to variable through the stack\n"
                                  "\tpop [%lu]\n\n",
                                  REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                                  *global_vars_cnt + 1 - (size_t) (-var_index));
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        size_t arg_index = 0;
        sscanf (buffer + *read_letters, "%lu", &arg_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tpush %lu\n"
                                  "\tpush %s\n"
                                  "\tsub\n"
                                  "\tpop %s\t ; Save value temporary to RBP\n"
                                  "\tpush [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop [%s+%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  REGISTERS [kRBPRegIndex],
                                  *cnt_func_args - arg_index,
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kVarBaseRegIndex], (size_t) var_index,
                                  REGISTERS [kRBPRegIndex]);
        }
        else
        {
            fprintf (output_file, "\tpush %s\t ; Save RBP for one memory access\n"
                                  "\tpush %lu\n"
                                  "\tpush %s\n"
                                  "\tsub\n"
                                  "\tpop %s\t ; Save value temporary to RBP\n"
                                  "\tpush [%s]\t ; Push argument to variable through the stack\n"
                                  "\tpop [%lu]\n"
                                  "\tpop %s\t ; Save value to RBP\n\n",
                                  REGISTERS [kRBPRegIndex],
                                  *cnt_func_args - arg_index,
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  *global_vars_cnt + 1 - (size_t) (-var_index),
                                  REGISTERS [kRBPRegIndex]);
        }

        return kDoneLang;
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignTmpSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (isdigit (*(buffer + *read_letters)))
    {
        double number = 0;
        sscanf (buffer + *read_letters, "%lg", &number);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        fprintf (output_file, "\tpush %lg\t ; Push the number to tmp through the stack\n"
                              "\tpop [%s+%lu]\n\n",
                              number,
                              REGISTERS [kTmpBaseRegIndex], result_tmp_index);

        return kDoneLang;
    }

    char prefix_name [kPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        long long var_index = 0;
        sscanf (buffer + *read_letters, "%lld", &var_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (*(buffer + *read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        if (var_index >= 0)
        {
            fprintf (output_file, "\tpush [%s+%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop [%s+%lu]\n\n",
                                  REGISTERS [kVarBaseRegIndex], (size_t) var_index,
                                  REGISTERS [kTmpBaseRegIndex], result_tmp_index);
        }
        else
        {
            fprintf (output_file, "\tpush [%lu]\t ; Push variable to tmp through the stack\n"
                                  "\tpop [%s+%lu]\n\n",
                                  *global_vars_cnt + 1 - (size_t) (-var_index),
                                  REGISTERS [kTmpBaseRegIndex], result_tmp_index);
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

        if (*(buffer + *read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        (*read_letters)++;
        *read_letters += skip_space_symbols (buffer + *read_letters);

        fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp to tmp through the stack\n"
                              "\tpop [%s+%lu]\n\n",
                              REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                              REGISTERS [kTmpBaseRegIndex], result_tmp_index);

        return kDoneLang;
    }

    return kInvalidAssigning;
}

static enum LangError AssignArgSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
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

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp %lu to argument %lu through the stack\n",
                          REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                          src_tmp_index, result_arg_index);

    return kDoneLang;
}

static enum LangError OperationSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    int operation_enum = 0;
    sscanf (buffer + *read_letters, "%d", &operation_enum);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kSepSym)
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

    if (*(buffer + *read_letters) != kSepSym)
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

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\tpush [%s+%lu]\t ; Push second operand\n"
                          "\tpush [%s+%lu]\t ; Push first operand\n"
                          "\t%s\n"
                          "\tpop [%s+%lu]\t ; Move tmp var counter reg value to the var counter reg\n\n",
                          REGISTERS [kTmpBaseRegIndex], second_operand_tmp_index,
                          REGISTERS [kTmpBaseRegIndex], first_operand_tmp_index,
                          kInstructionsSPU [operation_enum - 1],
                          REGISTERS [kTmpBaseRegIndex], result_tmp_index);

    return kDoneLang;
}

static enum LangError LabelSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    char label_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r^)]", label_name);
    *read_letters += strlen (label_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kBracketClose)
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

static enum LangError ReturnSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    const char* end_args = strchr (buffer + *read_letters, kBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    *read_letters = end_args - buffer;
    (*read_letters)++;

    fprintf (output_file, "\n\tpush [%s+%lu]\n"
                          "\tpop %s\t ; Save return value to the rax\n"
                          "\tpop %s\t ; Get RBP value back\n"
                          "\tpop %s\t ; Get tmp register value back\n"
                          "\tpop %s\t ; Get var register value back\n"
                          "ret\n",
                          REGISTERS [kTmpBaseRegIndex], ret_val_index,
                          REGISTERS [kRetValRegIndex],
                          REGISTERS [kRBPRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kVarBaseRegIndex]);

    return kDoneLang;
}

static enum LangError SysCallSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char sys_func_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^)^ ^\n^\t^\r^,]", sys_func_name);
    *read_letters += strlen (sys_func_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    const char* end_args = strchr (buffer + *read_letters, kBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    *read_letters = end_args - buffer;
    (*read_letters)++;

    fprintf (output_file, "%s\t ; System call\n", sys_func_name);

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_OUT_INDEX].Name) != 0)
    {
        fprintf (output_file, "\tpop [%s+%lu]\t ; Save return value to tmp\n",
                              REGISTERS [kTmpBaseRegIndex], ret_val_index);
    }

    if (strcmp (sys_func_name, kIR_SYS_CALL_ARRAY [SYSCALL_IN_INDEX].Name) == 0)
    {
        fprintf (output_file, "\tpop 0x\t ; Pop argument out of stack\n\n");
    }

    return kDoneLang;
}

static enum LangError GlobalVarsNumSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
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

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\tpush 0\n"
                          "\tpop %s\t ; Set zero to global variables counter register\n"
                          "\tpush %lu\n"
                          "\tpop %s\t ; Set value to tmp variables counter register\n\n",
                          REGISTERS [kVarBaseRegIndex],
                          cnt_glob_vars, REGISTERS [kTmpBaseRegIndex]);

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
