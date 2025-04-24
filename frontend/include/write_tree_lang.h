#ifndef WRITE_TREE_LANG_H
#define WRITE_TREE_LANG_H

#include <stdio.h>

#include "language.h"

static const char* const kMainKeyWord     = "MAIN:";
static const char* const kNumKeyWord      = "NUM:";
static const char* const kVarKeyWord      = "VAR:";
static const char* const kOpKeyWord       = "OP:";
static const char* const kCycleKeyWord    = "CYCLE:";
static const char* const kCondKeyWord     = "COND:";
static const char* const kTypeKeyWord     = "TYPE:";
static const char* const kUserFuncKeyWord = "USERFUNC:";

static const char* const kMainNodeWord    = "main";

static const char* const kAddNodeWord     = "ADD";
static const char* const kSubNodeWord     = "SUB";
static const char* const kMulNodeWord     = "MUL";
static const char* const kDivNodeWord     = "DIV";
static const char* const kPowNodeWord     = "POW";
static const char* const kSqrtNodeWord    = "SQRT";

static const char* const kSinNodeWord     = "sin";
static const char* const kCosNodeWord     = "cos";
static const char* const kTgNodeWord      = "tg";
static const char* const kCtgNodeWord     = "ctg";

static const char* const kLnNodeWord      = "ln";
static const char* const kLogNodeWord     = "log";

static const char* const kForNodeWord     = "for";
static const char* const kWhileNodeWord   = "while";
static const char* const kIfNodeWord      = "if";

static const char* const kAssignNodeWord  = "=";
static const char* const kParOpenNodeWord  = "(";
static const char* const kParCloseNodeWord = ")";
static const char* const kCurlyOpenNodeWord  = "{";
static const char* const kCurlyCloseNodeWord = "}";
static const char* const kCommandEndNodeWord = ";";
static const char* const kCommaNodeWord    = "COMMA";

static const char* const kInNodeWord  = "IN";
static const char* const kOutNodeWord = "OUT";

static const char* const kReturnNodeWord = "RET";

static const char* const kDoubleNodeWord = "DOUBLE";

enum LangError WriteDataBase (const node_t* const root, FILE* const output_file);

#endif // WRITE_TREE_LANG_H
