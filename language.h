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
    kMissTypeInGlobal           = 16,
    kCantCreateListOfTables     = 17,
    kTooManyVar                 = 18,
    kInvalidTokenType           = 19,
    kUndefinedVariable          = 20,
    kMissCommaFuncCall          = 21,
    kMissCommaInArgs            = 22,

    kCantDumpLang               = 23,
    kInvalidNodeTypeLangError   = 24,

    kInvalidPrefixDataBase      = 25,
    kMissValue                  = 26,

    kInvalidModeTypeLangError   = 27,
};

enum NodeType
{
    kNewNode = 0,

    kMainFunc = 1,

    kNum    = 2,
    kVar    = 3,
    kFunc   = 4,
    kArithm = 5,
    kCycle  = 6,
    kCond   = 7,
    kSym    = 8,
    kType   = 9,

    kUserFunc = 10,

    kEndToken = 11,

    kComp = 12,

    kInvalidNodeType = -1,
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
    kIf    = 15,

    kAssign                  = 16,
    kParenthesesBracketOpen  = 17,
    kParenthesesBracketClose = 18,
    kCurlyBracketOpen        = 19,
    kCurlyBracketClose       = 20,
    kCommandEnd              = 21,
    kComma                   = 22,

    kIn  = 23,
    kOut = 24,

    kReturn = 25,

    kDouble = 26,

    kMore     = 27,
    kMoreOrEq = 28,
    kLess     = 29,
    kLessOrEq = 30,
    kEqual    = 31,
    kNEqual   = 32,

    kInvalidFunc = -1,
};

typedef struct node
{
    enum NodeType type;

    union value
    {
        double      number;
        long long   variable;
        char        func_name [kWordLen];
        enum OpType operation;
    } value;

    struct node* parent;

    struct node* left;
    struct node* right;
} node_t;

#include "struct_lang.h"
#include "dump_lang.h"
#include "connect_tree_lang.h"
#include "read_lang.h"
#include "write_tree_lang.h"
#include "read_tree_lang.h"
#include "backend_lang.h"

#endif // LANGUAGE_H
