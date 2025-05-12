#if !(defined(WRITE_IR_H))
#define WRITE_IR_H

#include <stdio.h>

#include "language.h"
#include "struct_lang.h"

const size_t kStackStartSize = 200;
const int    kPoisonVal      = -1;

static const char* const kStartIR = "CALL_PENIS(88_tmp0, func_main_0, 0)\n"
                                    "CALL_PENIS(88_tmp1, func_hlt_0, 0)\n\n";

enum LangError WriteIR (const node_t* const root, FILE* const ir_file);

#endif // WRITE_IR_H
