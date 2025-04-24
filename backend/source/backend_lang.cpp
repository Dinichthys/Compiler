#include "backend_lang.h"
#include "language.h"

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"

static enum LangError PrintNode (const node_t* const root, FILE* const output_file);

static enum LangError PrintMain            (const node_t* const root, FILE* const output_file);
static enum LangError PrintNum             (const node_t* const root, FILE* const output_file);
static enum LangError PrintVar             (const node_t* const root, FILE* const output_file);
static enum LangError PrintFunc            (const node_t* const root, FILE* const output_file);
static enum LangError PrintArithm          (const node_t* const root, FILE* const output_file);
static enum LangError PrintSym             (const node_t* const root, FILE* const output_file);
static enum LangError PrintDefinition      (const node_t* const root, FILE* const output_file);
static enum LangError PrintUserFuncCall    (const node_t* const root, FILE* const output_file);
static enum LangError PrintArgs            (const node_t* const root, FILE* const output_file);
static enum LangError PrintUserFuncPattern (const node_t* const root, FILE* const output_file);

enum LangError GenerateAsm (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "push 0\n"
                          "pop ax\n"
                          "push 0\n"
                          "pop bx\n"
                          "push 0\n"
                          "pop cx\n"
                          "call main:\n"
                          "hlt\n\n");

    PrintNode (root, output_file);

    return kDoneLang;
}

static enum LangError PrintNode (const node_t* const root, FILE* const output_file)
{
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    if (root == NULL)
    {
        return kDoneLang;
    }

    switch (root->type)
    {
        case kMainFunc:
        {
            PrintMain (root, output_file);
            break;
        }
        case kNum:
        {
            PrintNum (root, output_file);
            break;
        }
        case kFunc:
        {
            PrintFunc (root, output_file);
            break;
        }
        case kArithm:
        {
            PrintArithm (root, output_file);
            break;
        }
        case kUserFunc:
        {
            PrintUserFuncCall (root, output_file);
            break;
        }
        case kVar:
        {
            PrintVar (root, output_file);
            break;
        }
        case kSym:
        {
            PrintSym (root, output_file);
            break;
        }
        case kType:
        {
            PrintDefinition (root, output_file);
            break;
        }

        case kEndToken:
            break;
        case kCycle: case kCond:
            break;

        case kNewNode: case kInvalidNodeType:
        default:
            return kInvalidNodeTypeLangError;
    }

    return kDoneLang;
}

static enum LangError PrintMain (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "\nmain:\n");

    const node_t* const subtree = (root->left == NULL) ? root->right : root->left;

    PrintNode (subtree, output_file);

    fprintf (output_file, "ret\n\n");

    return kDoneLang;
}

static enum LangError PrintNum (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "push %.3lf\n", root->value.number);

    return kDoneLang;
}

static enum LangError PrintVar (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "push [bx+%lld]\n", root->value.variable);

    return kDoneLang;
}

static enum LangError PrintFunc (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    const node_t* const subtree = (root->left == NULL) ? root->right : root->left;

    PrintNode (subtree, output_file);

    fprintf (output_file, "%s\n", EnumFuncToStr (root->value.operation));

    return kDoneLang;
}

static enum LangError PrintArithm (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    PrintNode (root->right, output_file);
    PrintNode (root->left, output_file);

    fprintf (output_file, "%s\n", EnumFuncToStr (root->value.operation));

    return kDoneLang;
}

static enum LangError PrintSym (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    if (root->value.operation == kAssign)
    {
        PrintNode (root->right, output_file);
        fprintf (output_file, "pop [bx+%lld]\n", root->left->value.variable);
    }

    if (root->value.operation == kCommandEnd)
    {
        PrintNode (root->right, output_file);
        PrintNode (root->left,  output_file);
    }

    return kDoneLang;
}

static enum LangError PrintUserFuncCall (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "push bx\n"
                          "pop dx\n"
                          "push bx\n"
                          "push cx\n"
                          "pop bx\n"
                          "push 0\n"
                          "pop cx\n");

    PrintArgs (root->right, output_file);

    fprintf (output_file, "call %s:\n"
                          "push bx\n"
                          "pop cx\n"
                          "pop bx\n", root->value.func_name);

    return kDoneLang;
}

static enum LangError PrintArgs (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "push [dx+%lld]\n"
                          "push bx\n"
                          "push cx\n"
                          "add\n"
                          "pop ex\n"
                          "pop [ex]\n"
                          "push cx+1\n"
                          "pop cx\n", root->value.variable);

    if (root->right != NULL)
    {
        PrintArgs (root->right, output_file);
    }

    return kDoneLang;
}

static enum LangError PrintDefinition (const node_t* const root, FILE* const output_file)
{
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    if ((root != NULL) && (root->type == kType) && (root->right->type == kUserFunc))
    {
        PrintUserFuncPattern (root->right, output_file);
        return PrintNode (root->left,  output_file);
    }

    if ((root != NULL) && (root->type == kType) && (root->right->type == kMainFunc))
    {
        PrintMain (root->right, output_file);
        return PrintNode (root->left, output_file);
    }

    if ((root != NULL) && (root->type == kType) && (root->right->type == kSym)
        && (root->right->value.operation == kAssign))
    {
        fprintf (output_file, "push cx+1\n"
                              "pop cx\n");
        PrintSym (root->right, output_file);
        return PrintNode (root->left, output_file);
    }

    while ((root != NULL) && (root->type == kType) && (root->right->type == kType))
    {
        fprintf (output_file, "push cx+1\n"
                              "pop cx\n");
        PrintDefinition (root->right, output_file);
    }

    return kDoneLang;
}

static enum LangError PrintUserFuncPattern (const node_t* const root, FILE* const output_file)
{
    ASSERT (root        != NULL, "Invalid argument root = %p\n", root);
    ASSERT (output_file != NULL, "Invalid argument output_file = %p\n", output_file);

    fprintf (output_file, "\n%s:\n", root->value.func_name);

    PrintDefinition (root->right, output_file);

    PrintNode (root->left, output_file);

    fprintf (output_file, "ret\n\n");

    return kDoneLang;
}
