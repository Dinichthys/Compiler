#if !(defined(TRANSLATION_SPU_H))
#define TRANSLATION_SPU_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"

const size_t kIRWordMaxLen    = 200;
const char   kBracketOpen     = '(';
const char   kIRCommentSymbol = '#';

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_SPU_H
