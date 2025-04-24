#ifndef DUMP_LANG_H
#define DUMP_LANG_H

#include <stdlib.h>

#include "language.h"

static const size_t kCommandTerminalDumpLen   = 200;

static const char* const kColorNodeMain     = "#F0F0F0";
static const char* const kColorNodeNum      = "#F0A0A0";
static const char* const kColorNodeVar      = "#A0F0A0";
static const char* const kColorNodeArithm   = "#A0A0A0";
static const char* const kColorNodeFunc     = "#A0A0F0";
static const char* const kColorNodeCycle    = "#A0D0F0";
static const char* const kColorNodeCond     = "#A0D0B0";
static const char* const kColorNodeSym      = "#D0D0D0";
static const char* const kColorNodeType     = "#C0A0C0";
static const char* const kColorNodeUserFunc = "#80F080";
static const char* const kColorNodeEnd      = "#A0A0A0";

static const char* const kColorEdgeLeft   = "blue";
static const char* const kColorEdgeRight  = "orange";
static const char* const kColorEdgeParent = "black";

enum LangError DumpLang   (node_t* const root);
const char* EnumFuncToStr (const enum OpType func);

#endif // DUMP_LANG_H
