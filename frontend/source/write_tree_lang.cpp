#include "write_tree_lang.h"
#include "language.h"

#include <stdlib.h>
#include <stdio.h>

#include "My_lib/Assert/my_assert.h"

static enum LangError PrintNode (const node_t* const root, FILE* const output_file, const size_t depth);

enum LangError WriteDataBase (const node_t* const root, FILE* const output_file)
{
    ASSERT (root         != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file  != NULL, "Invalid argument output_file = %p\n", output_file);

    return PrintNode (root, output_file, 0);
}

static enum LangError PrintNode (const node_t* const node, FILE* const output_file, const size_t depth)
{
    ASSERT (output_file  != NULL, "Invalid argument output_file = %p\n", output_file);

    for (size_t counter = 0; counter < depth; counter++)
    {
        fprintf (output_file, "\t");
    }

    if (node == NULL)
    {
        fprintf (output_file, "{}\n");
        return kDoneLang;
    }

//-------------------------------------------------------------------
    #define PRINT_NODE(type, data, node_type, specifier)                    \
        case type:                                                          \
        {                                                                   \
            fprintf (output_file, "{ %s " specifier " \n", node_type, data);\
            if ((node->left != NULL) || (node->right != NULL))              \
            {                                                               \
                PrintNode (node->right, output_file, depth + 1);            \
                PrintNode (node->left,  output_file, depth + 1);            \
            }                                                               \
            for (size_t counter = 0; counter < depth; counter++)            \
            {                                                               \
                fprintf (output_file, "\t");                                \
            }                                                               \
            fprintf (output_file, "}\n");                                   \
            return kDoneLang;                                               \
        }
//-------------------------------------------------------------------

    switch (node->type)
    {
        PRINT_NODE (kMainFunc, EnumFuncToStr (node->value.operation), kMainKeyWord,     "%s");
        PRINT_NODE (kNum,      node->value.number,                    kNumKeyWord,      "%.3lf");
        PRINT_NODE (kVar,      node->value.variable,                  kVarKeyWord,      "%lld");
        PRINT_NODE (kArithm,   EnumFuncToStr (node->value.operation), kOpKeyWord,       "%s");
        PRINT_NODE (kFunc,     EnumFuncToStr (node->value.operation), kOpKeyWord,       "%s");
        PRINT_NODE (kCycle,    EnumFuncToStr (node->value.operation), kCycleKeyWord,    "%s");
        PRINT_NODE (kCond,     EnumFuncToStr (node->value.operation), kCondKeyWord,     "%s");
        PRINT_NODE (kSym,      EnumFuncToStr (node->value.operation), kOpKeyWord,       "%s");
        PRINT_NODE (kType,     EnumFuncToStr (node->value.operation), kTypeKeyWord,     "%s");
        PRINT_NODE (kUserFunc, node->value.func_name,                 kUserFuncKeyWord, "%s");

        case kEndToken:
            return kDoneLang;

        case kNewNode:
            return kInvalidNodeTypeLangError;

        case kInvalidNodeType:
        default:
            return kInvalidNodeTypeLangError;
    }

    #undef PRINT_NODE


    return kDoneLang;
}
