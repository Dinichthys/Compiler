#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <stdio.h>

#include "language.h"

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_H
