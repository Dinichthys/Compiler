#if !(defined(PARSE_MODE_LANG_H))
#define PARSE_MODE_LANG_H

#include "language.h"
#include "parse_flags_lang.h"
#include "struct_lang.h"

enum LangError ParseMode (const settings_of_program_t* const set, node_t** const root);

#endif // PARSE_MODE_LANG_H
