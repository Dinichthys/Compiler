#include "language.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "My_lib/Logger/logging.h"
#include "My_lib/Assert/my_assert.h"

static enum LangError ReadNode           (node_t** const root, FILE* const input_file);
static void SkipSpaceSymbolsWithFirstSym (FILE* const input_file);
static node_t         WordToNode         (const char* const word);

enum LangError ReadDataBase (node_t** const root, FILE* const input_file)
{
    ASSERT (root       != NULL, "Invalid argument root = %p\n", root);
    ASSERT (input_file != NULL, "Invalid argument input_file = %p\n", input_file);

    return ReadNode (root, input_file);
}

static enum LangError ReadNode (node_t** const root, FILE* const input_file)
{
    ASSERT (root       != NULL, "Invalid argument root = %p\n", root);
    ASSERT (input_file != NULL, "Invalid argument input_file = %p\n", input_file);

    enum LangError result = kDoneLang;

    SkipSpaceSymbolsWithFirstSym (input_file);
    fseek (input_file, -1, SEEK_CUR);

    char word [kWordLen] = "";
    if (fscanf (input_file, "{ %100s", word) == 0)
    {
        return kMissValue;
    }

    LOG (kDebug, "Word = \"%s\"\n", word);

    if (strcmp (word, "}") == 0)
    {
        return kDoneLang;
    }

    if (strcmp (word, kMainKeyWord) == 0)
    {
        if (fscanf (input_file, "%s", word) == 0)
        {
            return kMissValue;
        }
        if (*root == NULL)
        {
            *root = AddNode ({.type = kMainFunc, {.operation = kMain}, .parent = NULL, .left = NULL, .right = NULL});
        }
        else
        {
            **root = {.type = kMainFunc, {.operation = kMain}, .parent = NULL, .left = NULL, .right = NULL};
        }
        result = ReadNode (&((*root)->right), input_file);
        CHECK_RESULT;
        result = ReadNode (&((*root)->left), input_file);
        CHECK_RESULT;
        SkipSpaceSymbolsWithFirstSym (input_file);
        return kDoneLang;
    }

    if (strcmp (word, kNumKeyWord) == 0)
    {
        double num = 0;
        if (fscanf (input_file, "%lf", &num) == 0)
        {
            return kMissValue;
        }
        if (*root == NULL)
        {
            *root = ADDNODE_NUM (num);
        }
        else
        {
            **root = {.type = kNum, {.number = num}, .parent = NULL, .left = NULL, .right = NULL};
        }
        SkipSpaceSymbolsWithFirstSym (input_file);
        return kDoneLang;
    }

    if (strcmp (word, kVarKeyWord) == 0)
    {
        long long var_num = 0;
        if (fscanf (input_file, "%lld", &var_num) == 0)
        {
            return kMissValue;
        }
        if (*root == NULL)
        {
            *root = AddNode ({.type = kVar, {.variable = var_num}, .parent = NULL, .left = NULL, .right = NULL});
        }
        else
        {
            **root = {.type = kVar, {.variable = var_num}, .parent = NULL, .left = NULL, .right = NULL};
        }
        SkipSpaceSymbolsWithFirstSym (input_file);
        return kDoneLang;
    }

    if ((strcmp (word, kOpKeyWord) == 0) || (strcmp (word, kCycleKeyWord) == 0) || (strcmp (word, kTypeKeyWord) == 0)
        || (strcmp (word, kCondKeyWord) == 0) || (strcmp (word, kUserFuncKeyWord) == 0))
    {
        if (fscanf (input_file, "%s", word) == 0)
        {
            return kMissValue;
        }
        if (*root == NULL)
        {
            *root = AddNode (WordToNode (word));
        }
        else
        {
            **root = WordToNode (word);
        }
        result = ReadNode (&((*root)->right), input_file);
        CHECK_RESULT;
        result = ReadNode (&((*root)->left), input_file);
        CHECK_RESULT;
        SkipSpaceSymbolsWithFirstSym (input_file);
        return kDoneLang;
    }

    return kInvalidPrefixDataBase;
}

static void SkipSpaceSymbolsWithFirstSym (FILE* const input_file)
{
    ASSERT (input_file != NULL, "Invalid argument input_file = %p\n");

    while (isspace (fgetc (input_file)))
    {
    }
}

static node_t WordToNode (const char* const word)
{
    ASSERT (word != NULL, "Invalid argument word = %p\n", word);

    #define CHECK_WORD(word_type, node_type, node_op)                                                        \
        if (strcmp (word, word_type) == 0)                                                                   \
        {                                                                                                    \
            return {.type = node_type, {.operation = node_op}, .parent = NULL, .left = NULL, .right = NULL}; \
        }

    LOG (kDebug, "Word = \"%s\"\n", word);

    CHECK_WORD (kMainNodeWord, kMainFunc, kMain);

    CHECK_WORD (kAddNodeWord,  kArithm, kAdd);
    CHECK_WORD (kSubNodeWord,  kArithm, kSub);
    CHECK_WORD (kMulNodeWord,  kArithm, kMul);
    CHECK_WORD (kDivNodeWord,  kArithm, kDiv);
    CHECK_WORD (kPowNodeWord,  kArithm, kPow);
    CHECK_WORD (kSqrtNodeWord, kArithm, kSqrt);

    CHECK_WORD (kSinNodeWord, kFunc, kSin);
    CHECK_WORD (kCosNodeWord, kFunc, kCos);
    CHECK_WORD (kTgNodeWord,  kFunc, kTg);
    CHECK_WORD (kCtgNodeWord, kFunc, kCtg);

    CHECK_WORD (kLnNodeWord,  kFunc, kLn);
    CHECK_WORD (kLogNodeWord, kFunc, kLog);

    CHECK_WORD (kForNodeWord,   kCycle, kFor);
    CHECK_WORD (kWhileNodeWord, kCycle, kWhile);

    CHECK_WORD (kIfNodeWord, kCond, kIf);

    CHECK_WORD (kAssignNodeWord,     kSym, kAssign);
    CHECK_WORD (kParOpenNodeWord,    kSym, kParenthesesBracketOpen);
    CHECK_WORD (kParCloseNodeWord,   kSym, kParenthesesBracketClose);
    CHECK_WORD (kCurlyOpenNodeWord,  kSym, kCurlyBracketOpen);
    CHECK_WORD (kCurlyCloseNodeWord, kSym, kCurlyBracketClose);
    CHECK_WORD (kCommandEndNodeWord, kSym, kCommandEnd);
    CHECK_WORD (kCommaNodeWord,      kSym, kComma);

    CHECK_WORD (kInNodeWord,  kFunc, kIn);
    CHECK_WORD (kOutNodeWord, kFunc, kOut);

    CHECK_WORD (kReturnNodeWord, kFunc, kReturn);

    CHECK_WORD (kDoubleNodeWord, kType, kDouble);

    #undef CHECK_WORD

    node_t ret_val = {.type = kUserFunc, .parent = NULL, .left = NULL, .right = NULL};

    strcpy (ret_val.value.func_name, word);

    return ret_val;
}
