#ifndef READ_LANG_H
#define READ_LANG_H

#include <stdio.h>
#include <stdlib.h>

#include "language.h"
#include "struct_lang.h"

static const size_t kVarMaxNum       = 200;
static const size_t kFuncMaxNum      = 200;
static const size_t kVarTablesMaxNum = 100;
static const size_t kTokenNumber     = 34;

static const char kCommentSymbol = '#';
static const char kEOF           = '\0';

__attribute_maybe_unused__ static const char* kRedTextTerminal    = "\033[1;31m";
__attribute_maybe_unused__ static const char* kNormalTextTerminal = "\033[0m";

typedef struct variables
{
    size_t var_num;
    char   var_table [kVarMaxNum] [kWordLen];
} variables_t;

typedef struct funcs
{
    size_t      func_num;
    func_node_t func_table [kFuncMaxNum];
} funcs_t;

typedef struct token
{
    enum NodeType type;

    union value
    {
        double      number;
        char        variable [kWordLen];
        enum OpType operation;
    } value;

    size_t line_pos;
    size_t number_of_line;
} token_t;

typedef struct token_pattern
{
    token_t token;
    const char* str_token;
} token_pattern_t;

const token_pattern_t kTokenTypes [kTokenNumber] {
//-------------------|                  Available token meaning                   |    Key word for the token    |
//-------------------|    type of node   |             meaning of node            |                              |
    {.token = {.type = kMainFunc, {.operation = kMain}, .line_pos = 0, .number_of_line = 0},                    .str_token = "Блатной"        },

    {.token = {.type = kArithm,   {.operation = kSub}, .line_pos = 0, .number_of_line = 0},                     .str_token = "вырезал"        },
    {.token = {.type = kArithm,   {.operation = kAdd}, .line_pos = 0, .number_of_line = 0},                     .str_token = "хапнул"         },
    {.token = {.type = kArithm,   {.operation = kDiv}, .line_pos = 0, .number_of_line = 0},                     .str_token = "твоя_доля"      },
    {.token = {.type = kArithm,   {.operation = kMul}, .line_pos = 0, .number_of_line = 0},                     .str_token = "Здравствуйте_я_сотрудник_Сбербанка"},
//-------------------нет в процессоре (снизу)
    {.token = {.type = kArithm,   {.operation = kPow}, .line_pos = 0, .number_of_line = 0},                     .str_token = "короновали"     },
//-------------------нет в процессоре (сверху)
    {.token = {.type = kFunc,     {.operation = kSqrt}, .line_pos = 0, .number_of_line = 0},                    .str_token = "кореш"          },

    {.token = {.type = kFunc,     {.operation = kSin}, .line_pos = 0, .number_of_line = 0},                     .str_token = "синенький"      },
    {.token = {.type = kFunc,     {.operation = kCos}, .line_pos = 0, .number_of_line = 0},                     .str_token = "костлявый"      },
//-------------------нет в процессоре (снизу)
    {.token = {.type = kFunc,     {.operation = kTg}, .line_pos = 0, .number_of_line = 0},                      .str_token = "штанга"         },
    {.token = {.type = kFunc,     {.operation = kCtg}, .line_pos = 0, .number_of_line = 0},                     .str_token = "котяра"         },

    {.token = {.type = kFunc,     {.operation = kLn}, .line_pos = 0, .number_of_line = 0},                      .str_token = "натурал"        },
    {.token = {.type = kFunc,     {.operation = kLog}, .line_pos = 0, .number_of_line = 0},                     .str_token = "лох"            },
//-------------------нет в процессоре (сверху)
    {.token = {.type = kCycle,    {.operation = kFor}, .line_pos = 0, .number_of_line = 0},                     .str_token = "мотать_срок"    },
    {.token = {.type = kCycle,    {.operation = kWhile}, .line_pos = 0, .number_of_line = 0},                   .str_token = "чалиться"       },

    {.token = {.type = kCond,     {.operation = kIf}, .line_pos = 0, .number_of_line = 0},                      .str_token = "УДО"            },
    {.token = {.type = kCond,     {.operation = kElse}, .line_pos = 0, .number_of_line = 0},                    .str_token = "дал_на_лапу"    },

    {.token = {.type = kSym,      {.operation = kAssign}, .line_pos = 0, .number_of_line = 0},                  .str_token = "отжал"          },
    {.token = {.type = kSym,      {.operation = kParenthesesBracketOpen}, .line_pos = 0, .number_of_line = 0},  .str_token = "заковали"       },
    {.token = {.type = kSym,      {.operation = kParenthesesBracketClose}, .line_pos = 0, .number_of_line = 0}, .str_token = "в_браслеты"     },
    {.token = {.type = kSym,      {.operation = kCurlyBracketOpen}, .line_pos = 0, .number_of_line = 0},        .str_token = "век"            },
    {.token = {.type = kSym,      {.operation = kCurlyBracketClose}, .line_pos = 0, .number_of_line = 0},       .str_token = "воли_не_видать" },
    {.token = {.type = kSym,      {.operation = kCommandEnd}, .line_pos = 0, .number_of_line = 0},              .str_token = "откинуться"     },
    {.token = {.type = kSym,      {.operation = kComma}, .line_pos = 0, .number_of_line = 0},                   .str_token = "перо_под_ребро" },

    {.token = {.type = kFunc,     {.operation = kIn}, .line_pos = 0, .number_of_line = 0},                      .str_token = "проставиться"   },
    {.token = {.type = kFunc,     {.operation = kOut}, .line_pos = 0, .number_of_line = 0},                     .str_token = "покукарекай"    },

    {.token = {.type = kRet,      {.operation = kReturn}, .line_pos = 0, .number_of_line = 0},                  .str_token = "АТАС_ШМОН"      },

    {.token = {.type = kType,     {.operation = kDouble}, .line_pos = 0, .number_of_line = 0},                  .str_token = "фраер"          },

    {.token = {.type = kComp,     {.operation = kMore}, .line_pos = 0, .number_of_line = 0},                    .str_token = "блатнее"        },
    {.token = {.type = kComp,     {.operation = kMoreOrEq}, .line_pos = 0, .number_of_line = 0},                .str_token = "больше_или_равно"},
    {.token = {.type = kComp,     {.operation = kLess}, .line_pos = 0, .number_of_line = 0},                    .str_token = "опущенный"       },
    {.token = {.type = kComp,     {.operation = kLessOrEq}, .line_pos = 0, .number_of_line = 0},                .str_token = "меньше_или_равно"},
    {.token = {.type = kComp,     {.operation = kEqual}, .line_pos = 0, .number_of_line = 0},                   .str_token = "ровный_поц"     },
    {.token = {.type = kComp,     {.operation = kNEqual}, .line_pos = 0, .number_of_line = 0},                  .str_token = "не_ровный_поц"  },
};

#undef TOKEN_PATTERN

enum LangError ReadProgram (const char* const input_file_name, node_t** const root);

#endif // READ_LANG_H
