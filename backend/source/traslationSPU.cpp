#include "translationSPU.h"
#include "language.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "key_words.h"

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

static enum LangError CallFuncSPU  (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError FuncBodySPU  (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError CondJumpSPU  (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError AssignSPU    (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError OperationSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError LabelSPU     (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError ReturnSPU    (const char* const buffer, size_t* const read_letters, FILE* const output_file);
static enum LangError SysCallSPU   (const char* const buffer, size_t* const read_letters, FILE* const output_file);

typedef enum LangError (*TranslateSpuFunc_t) (const char* const buffer, size_t* const read_letters, FILE* const output_file);

static const TranslateSpuFunc_t kTranslationArray [kIR_KEY_WORD_NUMBER] =
{
    CallFuncSPU,
    FuncBodySPU,
    CondJumpSPU,
    AssignSPU,
    OperationSPU,
    LabelSPU,
    ReturnSPU,
    SysCallSPU
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

    while (can_read)
    {
        char key_word [kIRWordMaxLen] = "";

        if (*(buffer + read_letters) == kIRCommentSymbol)
        {
            const char* end_comment = strchr (buffer, '\n');
            if (end_comment == NULL)
            {
                return kDoneLang;
            }
            read_letters = end_comment - buffer;
            read_letters++;
        }

        can_read = sscanf (buffer + read_letters, "%s", key_word);
        if (can_read == 0)
        {
            break;
        }
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
                result = kTranslationArray [index_kw] (buffer, &read_letters, output_file);
                CHECK_RESULT;
            }
        }
    }

    return kDoneLang;
}

static enum LangError CallFuncSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");

    size_t ret_val_index = 0;

    sscanf (buffer + *read_letters, TMP_PREFIX "%lu", &ret_val_index);



    return kDoneLang;
}

static enum LangError FuncBodySPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError CondJumpSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError AssignSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError OperationSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError LabelSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError ReturnSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}

static enum LangError SysCallSPU (const char* const buffer, size_t* const read_letters, FILE* const output_file)
{
    ASSERT (buffer       != NULL, "Invalid argument buffer\n");
    ASSERT (output_file  != NULL, "Invalid argument output_file\n");
    ASSERT (read_letters != NULL, "Invalid argument read_letters\n");
    return kDoneLang;
}
