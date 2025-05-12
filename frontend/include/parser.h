#if !(defined(PARSER_H))
#define PARSER_H

#include "language.h"
#include "read_lang.h"

enum LangError ParseTokens (token_t** const tokens, const char* const input_file_name);

#endif // PARSER_H
