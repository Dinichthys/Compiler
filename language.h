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

    kInvalidPatternOfIf         = 28,
    kInvalidPatternOfCycle      = 29,

    kCantWriteAssigning         = 30,
    kCantCreateStackArgs        = 31,
    kCantPushTMPVarCounter      = 32,
    kCantPopTMPVarCounter       = 33,
    kInvalidArgNum              = 34,
    kNoCommandEnd               = 35,
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

    kRet = 13,

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

typedef struct func_node
{
    char   func_name [kWordLen];
    size_t cnt_args;
} func_node_t;

typedef struct var_node
{
    char   variable [kWordLen];
    long long index;
} var_node_t;

typedef struct node
{
    enum NodeType type;

    union value
    {
        double number;
        var_node_t    variable;
        func_node_t   function;
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
#include "write_ir.h"
#include "read_tree_lang.h"
#include "backend_lang.h"

#endif // LANGUAGE_H
