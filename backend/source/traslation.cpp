#include "translation.h"
#include "language.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "key_words.h"

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

#define TMP_PREFIX "14_tmp"
#define ARG_PREFIX "88_tmp"
#define VAR_PREFIX "32_tmp"

#define CALL_FUNC(func_name, cnt_args)                                                                                    \
    fprintf (output_file, "call %s:\t ; Call function with %lu arguments\n", func_name, cnt_args);

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file)
{
    ASSERT (ir_file     != NULL, "Invalid argument ir_file in function GenerateASMSPUFromIR\n");
    ASSERT (output_file != NULL, "Invalid argument output_file in function GenerateASMSPUFromIR\n");

    char* buffer = ReadFileToBuffer (ir_file);
    if (buffer == NULL)
    {
        return kCantReadDataBase;
    }

    FREE_AND_NULL (buffer);
    return kDoneLang;
}

static enum LangError GenerateAsmSPU ( const char* const buffer, FILE* const output_file)
{
    ASSERT (buffer      != NULL, "Invalid argument buffer\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    char key_word [kIRWordMaxLen] = "";

    sscanf (buffer, "%s(", key_word);

    const char* buffer_ptr = strchr (buffer, kBracketOpen) + 1;
    if (buffer_ptr == NULL)
    {
        return kNoBracketsIR;
    }

    if (strcmp (key_word, kIRAssignKeyWord) == 0)
    {

    }

    if (strcmp (key_word, kIRCallKeyWord) == 0)
    {
        size_t tmp_index = 0;
        char func_name [kIRWordMaxLen] ="";
        sscanf (buffer_ptr, TMP_PREFIX "%lu, %s", &tmp_index);

    }

    if (strcmp (key_word, kIRFuncKeyWord) == 0)
    {
    }

    if (strcmp (key_word, kIRJumpKeyWord) == 0)
    {
    }

    if (strcmp (key_word, kIRLabelKeyWord) == 0)
    {

        fprintf (output_file, "\n%s:\n",);
    }

    if (strcmp (key_word, kIRRetKeyWord) == 0)
    {
    }

    if (strcmp (key_word, kIRSystemKeyWord) == 0)
    {
    }

    return kDoneLang;
}
