#ifndef BACKEND_LANG_H
#define BACKEND_LANG_H

#include "language.h"

#include <stdio.h>

enum LangError GenerateAsm (const node_t* const root, FILE* const output_file);

#endif // BACKEND_LANG_H
