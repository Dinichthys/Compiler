#include "read_lang.h"
#include "parser.h"
#include "lexer.h"

#include "struct_lang.h"
#include "connect_tree_lang.h"

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/helpful.h"

enum LangError ReadProgram (const char* const input_file_name, node_t** const root)
{
    ASSERT (input_file_name != NULL, "Invalid argument input file name = %p\n", input_file_name);
    ASSERT (root            != NULL, "Invalid argument root = %p\n",            root);

    enum LangError result = kDoneLang;

    token_t* tokens = NULL;

    result = ParseTokens (&tokens, input_file_name);
    if (result != kDoneLang)
    {
        FREE_AND_NULL (tokens);
        return result;
    }

    size_t token_index = 0;

    result = GetProgram (tokens, &token_index, root, input_file_name);
    FREE_AND_NULL (tokens);
    if (result != kDoneLang)
    {
        return result;
    }
    ConnectTree (*root);

    return result;
}
