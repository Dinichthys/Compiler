#include "translationSPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "language.h"
#include "program.h"

#include "key_words.h"
#include "operations.h"

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
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError FuncBodySPU      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError CondJumpSPU      (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError AssignSPU        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError AssignVarSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt, const size_t* const cnt_func_args);
static enum LangError AssignTmpSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        size_t* const global_vars_cnt);
static enum LangError AssignArgSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file);

static enum LangError OperationSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError LabelSPU         (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError ReturnSPU        (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError SysCallSPU       (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);
static enum LangError GlobalVarsNumSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                        __attribute_maybe_unused__ bool in_function,
                                        __attribute_maybe_unused__ char* const func_name,
                                        __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                        __attribute_maybe_unused__ size_t* const cnt_func_args);

static void SkipNumber (const char* const input_buf, size_t* const offset);

typedef enum LangError (*TranslateSpuFunc_t) (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args);

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

#define TMP_PREFIX "tmp"
#define VAR_PREFIX "var"
#define ARG_PREFIX "arg"

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

    while (can_read)
    {
        char key_word [kIRWordMaxLen] = "";

        if (*(buffer + read_letters) == kIRCommentSymbol)
        {
            const char* end_comment = strchr (buffer + read_letters, '\n');
            if (end_comment == NULL)
            {
                return kDoneLang;
            }
            read_letters = end_comment - buffer;
            read_letters++;
        }

        can_read = sscanf (buffer + read_letters, "%[^ ^\t^\n^(]", key_word);
        if (can_read == 0)
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
                }
                result = kTranslationArray [index_kw] (buffer, &read_letters, output_file, in_function,
                                                       func_name, &global_vars_cnt, &cnt_func_args);
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
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
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

    fprintf (output_file, "call %s:\t ; Calling function\n"
                          "\tpush %s\t ; Push return value to the stack\n"
                          "\tpop [%s+%lu]\t ; Save return value to tmp var\n",
                          call_func_name,
                          REGISTERS [kRetValRegIndex],
                          REGISTERS [kTmpBaseRegIndex], ret_val_index);

    return kDoneLang;
}

static enum LangError FuncBodySPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

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

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
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

    fprintf (output_file, "%s:\t ; Function body\n"
                          "\tpush %s\t ; Save variables counter register value\n"
                          "\tpush %s\t ; Save tmp variables counter register value\n\n"
                          "\tpush %s\t ; Save RBP\n"
                          "\tpush %s\n"
                          "\tpop %s\t ; Move tmp var counter reg value to the var counter reg\n\n"
                          "\tpush %s\n"
                          "\tpush %lu\n"
                          "\tadd\n"
                          "\tpop %s\t ; Make a stack frame for local variables\n\n",
                          func_name,
                          REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRBPRegIndex],
                          REGISTERS [kTmpBaseRegIndex], REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex], cnt_loc_vars, REGISTERS [kTmpBaseRegIndex]);

    return kDoneLang;
}

static enum LangError CondJumpSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

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
        (*read_letters)++;
    }
    else
    {
        sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &cond_res_tmp_index);
        SkipNumber (buffer, read_letters);
        *read_letters += skip_space_symbols (buffer + *read_letters);
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
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    char prefix_name [kPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kPrefixLen;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        return AssignVarSPU (buffer, read_letters, output_file, global_vars_cnt, cnt_func_args);
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        return AssignTmpSPU (buffer, read_letters, output_file, global_vars_cnt);
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        return AssignArgSPU (buffer, read_letters, output_file);
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignVarSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    size_t* const global_vars_cnt, const size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

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

    char prefix_name [kPrefixLen] = "";
    sscanf (buffer + *read_letters, "%3s", prefix_name);
    *read_letters += kPrefixLen;

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        size_t src_tmp_index = 0;
        sscanf (buffer + *read_letters, "%lu", &src_tmp_index);
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
            fprintf (output_file, "\tpush [%s-%lu]\t ; Push argument to variable through the stack\n"
                                  "\tpop [%s+%lu]\n\n",
                                  REGISTERS [kRBPRegIndex], *cnt_func_args - arg_index,
                                  REGISTERS [kVarBaseRegIndex], (size_t) var_index);
        }
        else
        {
            fprintf (output_file, "\tpush [%s-%lu]\t ; Push argument to variable through the stack\n"
                                  "\tpop [%lu]\n\n",
                                  REGISTERS [kRBPRegIndex], *cnt_func_args - arg_index,
                                  *global_vars_cnt + 1 - (size_t) (-var_index));
        }

        return kDoneLang;
    }

    return kDoneLang;
}

static enum LangError AssignTmpSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    size_t* const global_vars_cnt)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t result_tmp_index = 0;
    sscanf (buffer + *read_letters, "%lu", &result_tmp_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

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
    *read_letters += kPrefixLen;

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

static enum LangError AssignArgSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

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
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp to argument through the stack\n"
                          "\tpop [%s+%lu]\n\n",
                          REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                          REGISTERS [kRBPRegIndex], result_arg_index);

    return kDoneLang;
}

static enum LangError OperationSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                    __attribute_maybe_unused__ bool in_function,
                                    __attribute_maybe_unused__ char* const func_name,
                                    __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t result_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &result_tmp_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

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
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    size_t second_operand_tmp_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &second_operand_tmp_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

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
                                __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    char label_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^,^ ^\n^\t^\r]", label_name);
    *read_letters += strlen (label_name);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

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
                                 __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

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
                                    __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(buffer + *read_letters), *read_letters);

    size_t ret_val_index = 0;
    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);
    SkipNumber (buffer, read_letters);
    *read_letters += skip_space_symbols (buffer + *read_letters);

    if (*(buffer + *read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    (*read_letters)++;
    *read_letters += skip_space_symbols (buffer + *read_letters);

    char sys_func_name [kFuncMaxNameLenIR] = "";
    sscanf (buffer + *read_letters, "%[^)^ ^\n^\t^\r]", sys_func_name);
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

    return kDoneLang;
}

static enum LangError GlobalVarsNumSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file,
                                       __attribute_maybe_unused__ bool in_function,
                                       __attribute_maybe_unused__ char* const func_name,
                                       __attribute_maybe_unused__ size_t* const global_vars_cnt,
                                       __attribute_maybe_unused__ size_t* const cnt_func_args)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

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
