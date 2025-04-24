#include "dump_lang.h"

#include <stdio.h>
#include <stdlib.h>

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"
#include "My_lib/My_stdio/my_stdio.h"

static enum LangError PrintNodeInfoLang  (node_t* const node, FILE* const dump_file);
static enum LangError PrintTreeInfoLang  (node_t* const node, FILE* const dump_file);
static enum LangError PrintEdgesTreeLang (node_t* const root, FILE* const dump_file);

enum LangError DumpLang (node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument: root = %p\n", root);

    LOG (kDebug, "Function got arguments:\n"
                 "| root = %p |\n",
                 root);

    static size_t counter_dump = 0;

    FILE* html_dump_file = NULL;
    FOPEN (html_dump_file, "log/Dump.html", "a");
    if (html_dump_file == NULL)
    {
        return kCantDumpLang;
    }

    if (ftell (html_dump_file) == 0)
    {
        fprintf (html_dump_file, "<pre>\n<HR>\n");
    }

    FILE* dump_file = NULL;
    FOPEN (dump_file, "log/Dump.dot", "w");
    if (dump_file == NULL)
    {
        FCLOSE (html_dump_file);
        return kCantDumpLang;
    }

    fprintf (dump_file, "digraph\n{\n"
                        "\tfontname = \"Helvetica,Arial,sans-serif\";\n"
                        "\tnode [fontname = \"Helvetica,Arial,sans-serif\"];\n"
                        "\tgraph [rankdir = \"TB\"];\n"
                        "\tranksep = 1.5;\n"
                        "\tsplines = ortho;\n\n");

    PrintTreeInfoLang (root, dump_file);

    PrintEdgesTreeLang (root, dump_file);

    fputc ('}', dump_file);

    FCLOSE (dump_file);

    counter_dump++;

    char command [kCommandTerminalDumpLen] = "";

    sprintf (command, "dot -Tsvg log/Dump.dot -o log/Dump_%lu_.svg\n", counter_dump);

    if (system (command) == -1)
    {
        FCLOSE (html_dump_file);
        return kCantDumpLang;
    }

    fprintf (html_dump_file, "<img src = \"Dump_%lu_.svg\" width = 1450>\n<HR>\n", counter_dump);

    FCLOSE (html_dump_file);

    return kDoneLang;
}

static enum LangError PrintNodeInfoLang (node_t* const node, FILE* const dump_file)
{
    ASSERT (node      != NULL, "Invalid argument node = %p\n", node);
    ASSERT (dump_file != NULL, "Invalid argument dump_file = %p\n", dump_file);

//-------------------------------------------------------------------
    #define PRINT_NODE(type, color_node, data, specifier)           \
        case type:                                                  \
        {                                                           \
            fprintf (dump_file, "\t\"node%p\"\n\t[\n"               \
                                "\t\tlabel = \""                    \
                                "{ <parent> parent = %p |\n"        \
                                "\t\tnode = %p |"                      \
                                " <f0> " specifier " |"             \
                                "{ <f1> left = %p |"                \
                                " <f2> right = %p } }\"\n"          \
                                "\t\tshape = \"record\"\n"          \
                                "\t\tstyle = \"filled\"\n"          \
                                "\t\tfillcolor = \"%s\"\n"          \
                                "\t\tcolor = \"%s\"\n"              \
                                "\t];\n\n",                         \
                                (void*) node, (void*) node->parent, \
                                (void*) node, data,                 \
                                (void*) node->left,                 \
                                (void*) node->right,                \
                                color_node, color_node);            \
            return kDoneLang;                                       \
        }
//-------------------------------------------------------------------

    LOG (kDebug, "Function got arguments:\n"
                 "| node = %p | dump_file = %p |\n",
                 node, dump_file);

    switch (node->type)
    {
        PRINT_NODE (kMainFunc, kColorNodeMain,     EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kNum,      kColorNodeNum,      node->value.number,                    "%.3lf");
        PRINT_NODE (kVar,      kColorNodeVar,      node->value.variable,                  "%lld"  );
        PRINT_NODE (kArithm,   kColorNodeArithm,   EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kFunc,     kColorNodeFunc,     EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kCycle,    kColorNodeCycle,    EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kCond,     kColorNodeCond,     EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kSym,      kColorNodeSym,      EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kType,     kColorNodeType,     EnumFuncToStr (node->value.operation), "%s"   );
        PRINT_NODE (kUserFunc, kColorNodeUserFunc, node->value.func_name,                 "%s"   );
        PRINT_NODE (kEndToken, kColorNodeEnd,      "end",                                 "%s"   );

        case kNewNode:
            return kInvalidNodeTypeLangError;

        case kInvalidNodeType:
        default:
            return kInvalidNodeTypeLangError;
    }

    #undef PRINT_NODE

    return kDoneLang;
}

static enum LangError PrintTreeInfoLang (node_t* const root, FILE* const dump_file)
{
    ASSERT (root      != NULL, "Invalid argument root = %p\n", root);
    ASSERT (dump_file != NULL, "Invalid argument dump_file = %p\n", dump_file);

    LOG (kDebug, "Function got arguments:\n"
                 "| root = %p | dump_file = %p |\n",
                 root, dump_file);

    PrintNodeInfoLang (root, dump_file);

    if (root->left != NULL)
    {
        PrintTreeInfoLang (root->left, dump_file);
    }

    if (root->right != NULL)
    {
        PrintTreeInfoLang (root->right, dump_file);
    }

    return kDoneLang;
}

static enum LangError PrintEdgesTreeLang (node_t* const root, FILE* const dump_file)
{
    ASSERT (root      != NULL, "Invalid argument root = %p\n", root);
    ASSERT (dump_file != NULL, "Invalid argument dump_file = %p\n", dump_file);

    LOG (kDebug, "Function got arguments:\n"
                 "| root = %p | dump_file = %p |\n",
                 root, dump_file);

    if (root->left != NULL)
    {
        fprintf (dump_file, "\t\"node%p\":f1 -> \"node%p\""
                            "[color = \"black\", color = \"%s\"];\n\n",
                            (void*) root, (void*) root->left, kColorEdgeLeft);
        PrintEdgesTreeLang (root->left, dump_file);
    }

    if (root->right != NULL)
    {
        fprintf (dump_file, "\t\"node%p\":f2 -> \"node%p\""
                            "[color = \"black\", color = \"%s\"];\n\n",
                            (void*) root, (void*) root->right, kColorEdgeRight);
        PrintEdgesTreeLang (root->right, dump_file);
    }

    if (root->parent != NULL)
    {
        fprintf (dump_file, "\t\"node%p\":parent -> \"node%p\" "
                            "[color = \"black\", color = \"%s\"];\n\n",
                            (void*) root, (void*) root->parent, kColorEdgeParent);
    }

    return kDoneLang;
}

const char* EnumFuncToStr (const enum OpType func)
{
    switch (func)
    {
        case kMain: return kMainNodeWord;

        case kAdd:  return kAddNodeWord;
        case kSub:  return kSubNodeWord;
        case kMul:  return kMulNodeWord;
        case kDiv:  return kDivNodeWord;
        case kPow:  return kPowNodeWord;
        case kSqrt: return kSqrtNodeWord;

        case kSin: return kSinNodeWord;
        case kCos: return kCosNodeWord;
        case kTg:  return kTgNodeWord;
        case kCtg: return kCtgNodeWord;

        case kLn:  return kLnNodeWord;
        case kLog: return kLogNodeWord;

        case kFor:   return kForNodeWord;
        case kWhile: return kWhileNodeWord;
        case kIf:    return kIfNodeWord;

        case kAssign:                  return kAssignNodeWord;
        case kParenthesesBracketOpen:  return kParOpenNodeWord;
        case kParenthesesBracketClose: return kParCloseNodeWord;
        case kCurlyBracketOpen:        return kCurlyOpenNodeWord;
        case kCurlyBracketClose:       return kCurlyCloseNodeWord;
        case kCommandEnd:              return kCommandEndNodeWord;
        case kComma:                   return kCommaNodeWord;

        case kIn:  return kInNodeWord;
        case kOut: return kOutNodeWord;

        case kReturn: return kReturnNodeWord;

        case kDouble: return kDoubleNodeWord;

        case kInvalidFunc: return "Invalid function";

        default:
            return "Invalid enum element";
    }

}
