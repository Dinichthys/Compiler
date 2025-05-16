#ifndef TRANSLATION_NASM_H
#define TRANSLATION_NASM_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"

static const size_t kNASMIRWordMaxLen    = 200;
static const char   kNASMBracketOpen     = '(';
static const char   kNASMBracketClose    = ')';
static const char   kNASMIRCommentSymbol = '#';
static const char   kNASMSepSym          = ',';
static const char   kNASMTrueSymbol      = '1';

static const char* const kNASMMainFuncName   = "main";
static const char* const kNASMStartRamName   = "StartRam";
static const char* const kNASMStartLabelName = "_start";

static const size_t kNASMFuncMaxNameLenIR  = 200;
static const size_t kNASMPrefixLen         = 4;
static const size_t kNASMSavedRegistersNum = 3;
static const size_t kNASMQWordSize         = 8;

static const size_t kNASMRegistersNum         = 8;
static const size_t kNASMRetValRegIndex       = 0;
static const size_t kNASMTmpBaseRegIndex      = 1;
static const size_t kNASMVarBaseRegIndex      = 2;
static const size_t kNASMUnusedRegisterIndex  = 3;
static const size_t kNASMFirstOpRegIndex      = 4;
static const size_t kNASMSecondOpRegIndex     = 5;
static const size_t kNASMRSPRegIndex          = 6;
static const size_t kNASMRBPRegIndex          = 7;

static const char* const kNASMRegisters [kNASMRegistersNum] =
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

static const size_t kNASMIRArithmOperationsNum = 4;

static const char* const kInstructionsArithmNASM [kNASMIRArithmOperationsNum] =
{
    "addss",
    "subss",
    "mulss",
    "divss",
};

static const size_t kNASMIRLogicOperationsNum = 6;

static const size_t kNASMZFShift = 6;
static const size_t kNASMCFShift = 0;

static const char* const kInstructionsLogicNASM [kNASMIRLogicOperationsNum] =
{
    "cmpeqss",      // equal
    "cmpneqss",     // nequal
    "cmpltss",      // below
    "cmpless",      // below or equal
    "cmpnless",     // above
    "cmpnltss"      // above or equal
};

// static const char* const kInstructionsLogicNASM [kNASMIRLogicOperationsNum] =
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

enum LangError GenerateAsmNASMFromIR (FILE* const ir_file, FILE* const output_file);

#endif // TRANSLATION_NASM_H
