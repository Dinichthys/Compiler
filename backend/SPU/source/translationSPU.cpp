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

typedef struct current_position
{
    const char* buffer;
    size_t      read_letters;
    char        func_name [kFuncMaxNameLenIR];
    size_t      global_vars_cnt;
    size_t      cnt_func_args;
    size_t      max_tmp_counter;
} current_position_t;

static enum LangError GenerateAsmSPU (const char* const buffer, FILE* const output_file);

static enum LangError CallFuncSPU      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError FuncBodySPU      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError CondJumpSPU      (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignSPU        (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError AssignVarSPU     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignTmpSPU     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError AssignArgSPU     (current_position_t* const cur_pos, FILE* const output_file);

static enum LangError OperationSPU     (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError LabelSPU         (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError ReturnSPU        (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError SysCallSPU       (current_position_t* const cur_pos, FILE* const output_file);
static enum LangError GlobalVarsNumSPU (current_position_t* const cur_pos, FILE* const output_file);

static void SkipNumber (const char* const input_buf, size_t* const offset);

typedef enum LangError (*TranslateSpuFunc_t) (current_position_t* const cur_pos, FILE* const output_file);

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

    current_position_t cur_pos =
    {
        .buffer = buffer,
        .read_letters = read_letters,
        .func_name = "global",
        .global_vars_cnt = 0,
        .cnt_func_args = 0,
        .max_tmp_counter = 0,
    };

    while (can_read)
    {
        cur_pos.read_letters += skip_space_symbols (buffer + cur_pos.read_letters);
        char key_word [kIRWordMaxLen] = "";

        if (*(buffer + cur_pos.read_letters) == kIRCommentSymbol)
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
            LOG (kDebug, "Stop reading on index %lu\n"
                         "Current symbol = {%c}\n",
                         cur_pos.read_letters, *(buffer + cur_pos.read_letters));
            break;
        }

        LOG (kDebug, "Key word = \"%s\"\n", key_word);

        cur_pos.read_letters += strlen (key_word);
        cur_pos.read_letters += skip_space_symbols (buffer + cur_pos.read_letters);
        if (*(buffer + cur_pos.read_letters) != kBracketOpen)
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

static enum LangError CallFuncSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char call_func_name [kFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^)^ ^\n^\t^\r]", call_func_name);
    cur_pos->read_letters += strlen (call_func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

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
                          cur_pos->max_tmp_counter + 1,
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          call_func_name,
                          cur_pos->max_tmp_counter + 1,
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRetValRegIndex],
                          REGISTERS [kTmpBaseRegIndex], ret_val_index,
                          cnt_call_func_args + 1,
                          REGISTERS [kRSPRegIndex],
                          REGISTERS [kRSPRegIndex]);

    return kDoneLang;
}

static enum LangError FuncBodySPU (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r]", cur_pos->func_name);
    cur_pos->read_letters += strlen (cur_pos->func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &(cur_pos->cnt_func_args));
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t cnt_loc_vars = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &cnt_loc_vars);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
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
                          cur_pos->func_name,
                          REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRBPRegIndex],
                          REGISTERS [kTmpBaseRegIndex], REGISTERS [kVarBaseRegIndex],
                          REGISTERS [kTmpBaseRegIndex], cnt_loc_vars, REGISTERS [kTmpBaseRegIndex],
                          REGISTERS [kRSPRegIndex], REGISTERS [kRBPRegIndex]);

    return kDoneLang;
}

static enum LangError CondJumpSPU (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char label_name [kFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r]", label_name);
    cur_pos->read_letters += strlen (label_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    size_t cond_res_tmp_index = 0;
    bool cond_jump = (*(cur_pos->buffer + cur_pos->read_letters) != kTrueSymbol);
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (cond_jump)
    {
        fprintf (output_file, "\tpush [%s+%lu]\t ; Push the result of the condition to the stack\n"
                              "\tpush 1\t ; Push true\n\n"
                              "je %s_%s:\t ; Jump on label if the condition is true\n",
                              REGISTERS [kTmpBaseRegIndex], cond_res_tmp_index,
                              cur_pos->func_name, label_name);
    }
    else
    {
        fprintf (output_file, "jmp %s_%s:\t ; Jump on label\n", cur_pos->func_name, label_name);
    }

    return kDoneLang;
}

static enum LangError AssignSPU (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char prefix_name [kPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        return AssignVarSPU (cur_pos, output_file);
    }

    if (strcmp (prefix_name, TMP_PREFIX) == 0)
    {
        return AssignTmpSPU (cur_pos, output_file);
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        return AssignArgSPU (cur_pos, output_file);
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignVarSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char prefix_name [kPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kPrefixLen - 1;

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

        if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

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
                                  cur_pos->global_vars_cnt + 1 - (size_t) (-var_index));
        }

        return kDoneLang;
    }

    if (strcmp (prefix_name, ARG_PREFIX) == 0)
    {
        size_t arg_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lu", &arg_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

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
                                  cur_pos->cnt_func_args - arg_index,
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
                                  cur_pos->cnt_func_args - arg_index,
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  REGISTERS [kRBPRegIndex],
                                  cur_pos->global_vars_cnt + 1 - (size_t) (-var_index),
                                  REGISTERS [kRBPRegIndex]);
        }

        return kDoneLang;
    }

    return kInvalidPrefixIR;
}

static enum LangError AssignTmpSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (isdigit (*(cur_pos->buffer + cur_pos->read_letters)))
    {
        double number = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lg", &number);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        fprintf (output_file, "\tpush %lg\t ; Push the number to tmp through the stack\n"
                              "\tpop [%s+%lu]\n\n",
                              number,
                              REGISTERS [kTmpBaseRegIndex], result_tmp_index);

        return kDoneLang;
    }

    char prefix_name [kPrefixLen] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%3s", prefix_name);
    cur_pos->read_letters += kPrefixLen - 1;

    if (strcmp (prefix_name, VAR_PREFIX) == 0)
    {
        long long var_index = 0;
        sscanf (cur_pos->buffer + cur_pos->read_letters, "%lld", &var_index);
        SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

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
                                  cur_pos->global_vars_cnt + 1 - (size_t) (-var_index),
                                  REGISTERS [kTmpBaseRegIndex], result_tmp_index);
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

        if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
        {
            return kNoBracketsIR;
        }
        cur_pos->read_letters++;
        cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

        fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp to tmp through the stack\n"
                              "\tpop [%s+%lu]\n\n",
                              REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                              REGISTERS [kTmpBaseRegIndex], result_tmp_index);

        return kDoneLang;
    }

    return kInvalidAssigning;
}

static enum LangError AssignArgSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    fprintf (output_file, "\tpush [%s+%lu]\t ; Push tmp %lu to argument %lu through the stack\n",
                          REGISTERS [kTmpBaseRegIndex], src_tmp_index,
                          src_tmp_index, result_arg_index);

    return kDoneLang;
}

static enum LangError OperationSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    int operation_enum = 0;
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%d", &operation_enum);
    SkipNumber (cur_pos->buffer, &(cur_pos->read_letters));
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

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

static enum LangError LabelSPU (current_position_t* const cur_pos, FILE* const output_file)
{
    ASSERT (cur_pos     != NULL, "Invalid argument cur_pos\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    LOG (kDebug, "Current symbol       = {%c}\n"
                 "Already read letters = %lu\n",
                 *(cur_pos->buffer + cur_pos->read_letters), cur_pos->read_letters);

    char label_name [kFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^,^ ^\n^\t^\r^)]", label_name);
    cur_pos->read_letters += strlen (label_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    LOG (kDebug, "Label \"%s\" was analyzed\n", label_name);

    fprintf (output_file, "%s_%s:\t ; Label in function\n", cur_pos->func_name, label_name);

    return kDoneLang;
}

static enum LangError ReturnSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    const char* end_args = strchr (cur_pos->buffer + cur_pos->read_letters, kBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters = end_args - cur_pos->buffer;
    cur_pos->read_letters++;

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

static enum LangError SysCallSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kSepSym)
    {
        return kNoSeparateSymbol;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    char sys_func_name [kFuncMaxNameLenIR] = "";
    sscanf (cur_pos->buffer + cur_pos->read_letters, "%[^)^ ^\n^\t^\r^,]", sys_func_name);
    cur_pos->read_letters += strlen (sys_func_name);
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    const char* end_args = strchr (cur_pos->buffer + cur_pos->read_letters, kBracketClose);
    if (end_args == NULL)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters = end_args - cur_pos->buffer;
    cur_pos->read_letters++;

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

static enum LangError GlobalVarsNumSPU (current_position_t* const cur_pos, FILE* const output_file)
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

    if (*(cur_pos->buffer + cur_pos->read_letters) != kBracketClose)
    {
        return kNoBracketsIR;
    }
    cur_pos->read_letters++;
    cur_pos->read_letters += skip_space_symbols (cur_pos->buffer + cur_pos->read_letters);

    fprintf (output_file, "\tpush 0\n"
                          "\tpop %s\t ; Set zero to global variables counter register\n"
                          "\tpush %lu\n"
                          "\tpop %s\t ; Set value to tmp variables counter register\n\n",
                          REGISTERS [kVarBaseRegIndex],
                          cnt_glob_vars, REGISTERS [kTmpBaseRegIndex]);

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
