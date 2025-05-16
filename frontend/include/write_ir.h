#if !(defined(WRITE_IR_H))
#define WRITE_IR_H

#include <stdio.h>

#include "language.h"
#include "struct_lang.h"

const size_t kStackStartSize = 200;
const int    kPoisonVal      = -1;

enum LangError WriteIR (const node_t* const root, FILE* const ir_file);

#endif // WRITE_IR_H
