#if !(defined(TRANSLATION_H))
#define TRANSLATION_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"

const size_t kIRWordMaxLen = 200;
const char   kBracketOpen  = '(';

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_H
