#include "connect_tree_lang.h"

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"

enum LangError ConnectTree (node_t* const root)
{
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    enum LangError result = kDoneLang;

    node_t* parent = root;
    node_t* kid = root->left;

    if ((kid != NULL) && (kid->parent != parent))
    {
        kid->parent = parent;
        result = ConnectTree (kid);
    }

    kid = root->right;

    if ((kid != NULL) && (kid->parent != parent))
    {
        kid->parent = parent;
        result = ConnectTree (kid);
    }

    return result;
}
