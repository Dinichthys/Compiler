#if !(defined(LEXER_H))
#define LEXER_H

#include <stdlib.h>

#include "language.h"
#include "read_lang.h"
#include "struct_lang.h"

enum LangError GetProgram (const token_t* const tokens, size_t* const token_index, node_t** const node,
                           const char* const file_name);

#endif // LEXER_H
