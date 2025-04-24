#include "struct_lang.h"

#include <stdlib.h>
#include <string.h>

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"
#include "My_lib/My_stdio/my_stdio.h"
#include "My_lib/helpful.h"

node_t* TreeCtor (void)
{
    node_t* root = (node_t*) calloc (1, sizeof (node_t));

    if (root == NULL)
    {
        return NULL;
    }

    root->type = kNewNode;

    root->parent = root->left = root->right = NULL;

    return root;
}

enum LangError TreeDtor (node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    root->parent = NULL;

    if (root->left != NULL)
    {
        TreeDtor (root->left);
    }

    if (root->right != NULL)
    {
        TreeDtor (root->right);
    }

    free (root);

    return kDoneLang;
}

node_t* AddNode (const node_t set_val)
{
    node_t* node = NULL;

    node = (node_t*) calloc (1, sizeof (node_t));
    if (node == NULL)
    {
        return NULL;
    }
    *(node) = set_val;

    return node;
}

const char* EnumErrorToStr (const enum LangError error)
{
    #define CASE(error)             \
        case error:                 \
        {                           \
            return #error;          \
        }

    switch (error)
    {
        CASE (kDoneLang);

        CASE (kCantAddNode);
        CASE (kCantCreateNode);

        CASE (kInvalidFileName);
        CASE (kCantReadDataBase);
        CASE (kCantCallocInputBuffer);
        CASE (kCantCallocTokenBuf);
        CASE (kCantOpenDataBase);
        CASE (kNoBraceCloser);
        CASE (kUndefinedFuncForRead);
        CASE (kSyntaxError);
        CASE (kCantDivideByZero);
        CASE (kInvalidTokenForExpression);
        CASE (kInvalidCommand);
        CASE (kInvalidPatternOfFunc);
        CASE (kInvalidAssigning);
        CASE (kMissTypeInGlobal);
        CASE (kCantCreateListOfTables);
        CASE (kTooManyVar);
        CASE (kInvalidTokenType);
        CASE (kUndefinedVariable);
        CASE (kMissCommaFuncCall);
        CASE (kMissCommaInArgs);

        CASE (kCantDumpLang);
        CASE (kInvalidNodeTypeLangError);

        CASE (kInvalidPrefixDataBase);
        CASE (kMissValue);

        CASE (kInvalidModeTypeLangError);

        default:
            return "Invalid enum element";
    }

    #undef CASE
}
