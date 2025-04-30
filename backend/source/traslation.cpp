#include "translation.h"
#include "language.h"

#include <stdio.h>
#include <stdlib.h>

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

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
