#if !(defined(TRANSLATION_SPU_H))
#define TRANSLATION_SPU_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"

static const size_t kIRWordMaxLen    = 200;
static const char   kBracketOpen     = '(';
static const char   kBracketClose    = ')';
static const char   kIRCommentSymbol = '#';
static const char   kSepSym          = ',';
static const char   kTrueSymbol      = '1';

static const size_t kFuncMaxNameLenIR  = 200;
static const size_t kPrefixLen         = 4;
static const size_t kSavedRegistersNum = 3;

static const size_t kRetValRegIndex       = 1;
static const size_t kTmpBaseRegIndex      = 2;
static const size_t kVarBaseRegIndex      = 3;
static const size_t kGlobalVarEndRegIndex = 4;
static const size_t kUnusedRegIndex       = 5;
static const size_t kRSPRegIndex          = 8;
static const size_t kRBPRegIndex          = 9;

static const size_t kIROperationsNum = 10;

const char* const kInstructionsSPU [kIROperationsNum] =
{
    "add",
    "sub",
    "mul",
    "div",
    "eq",
    "neq",
    "less",
    "lessEq",
    "more",
    "moreEq"
};

enum LangError GenerateAsmSPUFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_SPU_H
