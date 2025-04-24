#ifndef READ_TREE_LANG_H
#define READ_TREE_LANG_H

#include <stdio.h>

#include "language.h"

enum LangError ReadDataBase (node_t** const root, FILE* const input_file);

#endif // READ_TREE_LANG_H
