#include "middleend.h"

#include <stdio.h>

#include "language.h"
#include "read_ir.h"
#include "list.h"


#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"


enum IRError
{
    kDoneIR         = 0,

    kCantReadBuffer = 1,

};

enum IRError OptimizeIR (FILE* const input_file, FILE* const output_file)
{
    ASSERT (input_file  != NULL, "Invalid argument input_file\n");
    ASSERT (output_file != NULL, "Invalid argument output_file\n");

    char* buffer = ReadFileToBuffer (input_file);
    if (buffer == NULL)
    {
        return kCantReadBuffer;
    }

    list_t list = {};
    enum LangError result = ReadIR (buffer, &list);
    FREE_AND_NULL (buffer);
    if (result != kDoneLang)
    {
        return (enum IRError) result;
    }
    return OptimizeIRList (&list);
}
