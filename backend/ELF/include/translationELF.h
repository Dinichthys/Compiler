#ifndef TRANSLATION_ELF_H
#define TRANSLATION_ELF_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"

static const size_t kELFIRWordMaxLen    = 200;
static const char   kELFBracketOpen     = '(';
static const char   kELFBracketClose    = ')';
static const char   kELFIRCommentSymbol = '#';
static const char   kELFSepSym          = ',';
static const char   kELFTrueSymbol      = '1';

static const char* const kELFMainFuncName   = "main";
static const char* const kELFStartRamName   = "StartRam";
static const char* const kELFStartLabelName = "_start";

static const size_t kELFFuncMaxNameLenIR  = 200;
static const size_t kELFPrefixLen         = 4;
static const size_t kELFSavedRegistersNum = 3;
static const size_t kELFQWordSize         = 8;

static const size_t kELFRegistersNum         = 8;
static const size_t kELFRetValRegIndex       = 0;
static const size_t kELFTmpBaseRegIndex      = 1;
static const size_t kELFVarBaseRegIndex      = 2;
static const size_t kELFUnusedRegisterIndex  = 3;
static const size_t kELFFirstOpRegIndex      = 4;
static const size_t kELFSecondOpRegIndex     = 5;
static const size_t kELFRSPRegIndex          = 6;
static const size_t kELFRBPRegIndex          = 7;

static const char* const kELFRegisters [kELFRegistersNum] =
{
    "rax",
    "rbx",
    "rcx",
    "rdx",
    "xmm0",
    "xmm1",
    "rsp",
    "rbp"
};

static const size_t kELFIRArithmOperationsNum = 4;

static const char* const kInstructionsArithmELF [kELFIRArithmOperationsNum] =
{
    "addss",
    "subss",
    "mulss",
    "divss",
};

static const size_t kELFIRLogicOperationsNum = 6;

static const size_t kELFZFShift = 6;
static const size_t kELFCFShift = 0;

static const char* const kInstructionsLogicELF [kELFIRLogicOperationsNum] =
{
    "cmpeqss",      // equal
    "cmpneqss",     // nequal
    "cmpltss",      // below
    "cmpless",      // below or equal
    "cmpnless",     // above
    "cmpnltss"      // above or equal
};

// static const char* const kInstructionsLogikELF [kELFIRLogicOperationsNum] =
// {
//     "\tshl ah, 1\n"
//     "\tshr ah, 7\t ; AH = ZF\n",        // equal
//
//     "\tshl ah, 1\n"
//     "\tnot ah\n"
//     "\tshr ah, 7\t ; AH = !ZF\n",       // nequal
//
//     "\tand ah, 1\t ; AH = CF\n",        // below
//
//     "\tmov dh, ah\n"
//     "\tand ah, 1\t ; AH = CF\n"
//     "\tshr dh, 6\n"
//     "\tand dh, 1\t ; DH = ZF\n"
//     "\tor ah, dh\t ; AH = CF || ZF",    // below or equal
//
//     "\tmov dh, ah\n"
//     "\tshl ah, 7\n"
//     "\tnot ah\n"
//     "\tshr ah, 7\t ; AH = !CF\n"
//     "\tshl dh, 1\n"
//     "\tnot dh\n"
//     "\tshr dh, 7\t ; DH = !ZF\n"
//     "\tand ah, dh\t ; AH = !CF && !ZF", // above
//
//     "\tshl ah, 7\n"
//     "\tnot ah\n"
//     "\tshr ah, 7\t ; AH = !CF\n"        // above or equal
//};

enum LangError GenerateAsmELFFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_ELF_H
