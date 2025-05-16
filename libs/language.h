#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <stdlib.h>

static const double kAccuracy   = 0.001;
static const size_t kWordLen    = 200;

enum LangError
{
    kDoneLang                   = 0,

    kCantAddNode                = 1,
    kCantCreateNode             = 2,

    kInvalidFileName            = 3,
    kCantReadDataBase           = 4,
    kCantCallocInputBuffer      = 5,
    kCantCallocTokenBuf         = 6,
    kCantOpenDataBase           = 7,
    kNoBraceCloser              = 8,
    kUndefinedFuncForRead       = 9,
    kSyntaxError                = 10,
    kCantDivideByZero           = 11,
    kInvalidTokenForExpression  = 12,
    kInvalidAssigning           = 13,
    kInvalidCommand             = 14,
    kInvalidPatternOfFunc       = 15,
    kDoubleFuncDefinition       = 16,
    kMissTypeInGlobal           = 17,
    kCantCreateListOfTables     = 18,
    kTooManyVar                 = 19,
    kInvalidTokenType           = 20,
    kUndefinedVariable          = 21,
    kMissCommaFuncCall          = 22,
    kMissCommaInArgs            = 23,

    kCantDumpLang               = 24,
    kInvalidNodeTypeLangError   = 25,

    kInvalidPrefixDataBase      = 26,
    kMissValue                  = 27,

    kInvalidModeTypeLangError   = 28,

    kInvalidPatternOfIf         = 29,
    kInvalidPatternOfCycle      = 30,

    kCantWriteAssigning         = 31,
    kCantCreateStackArgs        = 32,
    kCantPushTMPVarCounter      = 33,
    kCantPopTMPVarCounter       = 34,
    kInvalidArgNum              = 35,
    kNoCommandEnd               = 36,

    kNoBracketsIR               = 37,
    kInvalidSyscall             = 38,
    kInvalidOperation           = 39,
    kNoSeparateSymbol           = 40,
    kInvalidPrefixIR            = 41,
};

enum OpType
{
    kMain = 0,

    kAdd  = 1,
    kSub  = 2,
    kMul  = 3,
    kDiv  = 4,
    kPow  = 5,
    kSqrt = 6,

    kSin  = 7,
    kCos  = 8,
    kTg   = 9,
    kCtg  = 10,

    kLn   = 11,
    kLog  = 12,

    kFor   = 13,
    kWhile = 14,

    kIf   = 15,
    kElse = 16,

    kAssign                  = 17,
    kParenthesesBracketOpen  = 18,
    kParenthesesBracketClose = 19,
    kCurlyBracketOpen        = 20,
    kCurlyBracketClose       = 21,
    kCommandEnd              = 22,
    kComma                   = 23,

    kIn  = 24,
    kOut = 25,

    kReturn = 26,

    kDouble = 27,

    kMore     = 28,
    kMoreOrEq = 29,
    kLess     = 30,
    kLessOrEq = 31,
    kEqual    = 32,
    kNEqual   = 33,

    kUndefinedNode = 34,

    kInvalidFunc = -1,
};

#endif // LANGUAGE_H
