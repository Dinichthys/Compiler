#include "parser.h"

#include "struct_lang.h"
#include "read_lang.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#include "MyLib/Assert/my_assert.h"
#include "MyLib/Logger/logging.h"
#include "MyLib/My_stdio/my_stdio.h"
#include "MyLib/helpful.h"

#define CHECK_RESULT                \
        if (result != kDoneLang)    \
        {                           \
            return result;          \
        }

static enum LangError ReadBufferFromFile (char** const  input_buf, const char* const input_file_name,
                                          size_t* const file_size_ret);

static void   SkipNumber  (const char* const input_buf, size_t* const offset);
static size_t SkipSpace   (const char* const input_buf, size_t* const counter_nl, size_t* const line_pos);

static token_t IdentifyToken  (char* const word);

enum LangError ParseTokens (token_t** const tokens, const char* const input_file_name)
{
    ASSERT (tokens          != NULL, "Invalid argument tokens array = %p\n",       tokens);
    ASSERT (input_file_name != NULL, "Invalid argument Name of input file = %p\n", input_file_name);

    enum LangError result = kDoneLang;

    char* input_buf = NULL;
    size_t file_size = 0;

    result = ReadBufferFromFile (&input_buf, input_file_name, &file_size);
    CHECK_RESULT

    *tokens = (token_t*) calloc (file_size + 1, sizeof (token_t));
    if (*tokens == NULL)
    {
        FREE_AND_NULL (input_buf);
        return kCantCallocTokenBuf;
    }

    size_t offset = 0;
    size_t token_index = 0;
    size_t counter_nl = 1;
    size_t line_pos = 1;
    bool in_comment = false;

    offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

    while (input_buf [offset] != kEOF)
    {
        LOG (kDebug, "Input buffer        = %p\n"
                     "Offset              = %lu\n"
                     "Position in line    = %lu\n"
                     "Number of new lines = %lu\n"
                     "Run time symbol     = {%c}\n",
                     input_buf, offset, line_pos, counter_nl, input_buf [offset]);

        if (input_buf [offset] == kCommentSymbol)
        {
            in_comment = !in_comment;
            offset++;
            line_pos++;
        }

        if (in_comment)
        {
            if (input_buf [offset] == '\n')
            {
                line_pos = 0;
                counter_nl++;
            }
            offset++;
            line_pos++;
            continue;
        }

        offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

        if ((isdigit (input_buf [offset])) || (input_buf [offset] == '-'))
        {
            double number = 0;
            sscanf (input_buf + offset, "%lf", &number);

            (*tokens) [token_index++] = {.type = kNum, {.number = number},
                                         .line_pos = line_pos, .number_of_line = counter_nl};

            size_t old_offset = offset;
            SkipNumber (input_buf, &offset);
            line_pos += offset - old_offset;

            offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

            continue;
        }

        LOG (kDebug, "Current symbol = {%c}\n", input_buf [offset]);

        char word [kWordLen] = "";
        sscanf (input_buf + offset, "%s", word);
        word [kWordLen - 1] = '\0';

        (*tokens) [token_index] = IdentifyToken (word);
        LOG (kDebug, "Length of the word = %lu\n", strlen (word));
        if ((*tokens) [token_index].type == kInvalidNodeType)           // If the token is not a key word
        {
            (*tokens) [token_index].type = kVar;
            strcpy ((*tokens) [token_index].value.variable, word);
        }
        (*tokens) [token_index].line_pos       = line_pos;
        (*tokens) [token_index].number_of_line = counter_nl;
        token_index++;

        offset += strlen (word);
        line_pos += strlen (word);

        offset += SkipSpace (input_buf + offset, &counter_nl, &line_pos);

        continue;
    }

    FREE_AND_NULL (input_buf);

    (*tokens) [token_index].type = kEndToken;

    return kDoneLang;
}

static enum LangError ReadBufferFromFile (char** const  input_buf, const char* const input_file_name,
                                          size_t* const file_size_ret)
{
    ASSERT (input_buf       != NULL, "Invalid argument Input buffer addres = %p\n", input_buf);
    ASSERT (input_file_name != NULL, "Invalid argument Name of input file = %p\n",  input_file_name);

    FILE* input_file = NULL;
    FOPEN (input_file, input_file_name, "r");
    if (input_file == NULL)
    {
        return kCantOpenDataBase;
    }

    size_t file_size = size_of_file (input_file);

    LOG (kDebug, "Size of file = %lu\n",
                 file_size);

    if (file_size_ret != NULL)
    {
        *file_size_ret = file_size;
    }

    if (file_size == 0)
    {
        return kCantReadDataBase;
    }

    *input_buf = (char*) calloc (file_size + 1, sizeof (char));
    if (*input_buf == NULL)
    {
        return kCantCallocInputBuffer;
    }

    LOG (kDebug, "Input buffer = %p\n"
                 "Size of file = %lu\n",
                 input_buf, file_size);

    if (fread (*input_buf, sizeof (char), file_size, input_file) == 0)
    {
        FREE_AND_NULL (*input_buf);
        return kCantReadDataBase;
    }
    FCLOSE (input_file);

    (*input_buf) [file_size] = kEOF;

    return kDoneLang;
}

static void SkipNumber (const char* const input_buf, size_t* const offset)
{
    ASSERT (input_buf != NULL, "Invalid argument input_buf = %p\n", input_buf);
    ASSERT (offset    != NULL, "Invalid argument offset = %p\n",    offset);

    LOG (kDebug, "Input buffer    = %p\n"
                 "Offset          = %lu\n"
                 "Run time symbol = {%c}\n",
                 input_buf, *offset, input_buf [*offset]);

    if (input_buf [*offset] == '-')
    {
        (*offset)++;
    }

    while (isdigit (input_buf [*offset]))
    {
        (*offset)++;
    }

    if (input_buf [*offset] == '.')
    {
        (*offset)++;
    }

    while (isdigit (input_buf [*offset]))
    {
        (*offset)++;
    }
}

static size_t SkipSpace (const char* const input_buf, size_t* const counter_nl, size_t* const line_pos)
{
    ASSERT (input_buf  != NULL, "Invalid argument: Pointer on input buffer = %p\n",         input_buf);
    ASSERT (counter_nl != NULL, "Invalid argument: Pointer on counter of new lines = %p\n", counter_nl);
    ASSERT (line_pos   != NULL, "Invalid argument: Pointer on line position = %p\n",        line_pos);

    size_t offset = 0;

    while (isspace (input_buf [offset]))
    {
        if (input_buf [offset] == '\n')
        {
            (*counter_nl)++;
            *line_pos = 0;
        }
        offset++;
        (*line_pos)++;
    }

    return offset;
}

static token_t IdentifyToken (char* const word)
{
    ASSERT (word != NULL, "Invalid argument func = %p\n", word);

    LOG (kDebug, "Word = \"%s\"\n", word);

    for (size_t index = 0; index < kTokenNumber; index++)
    {
        if (strcmp (word, kTokenTypes [index].str_token) == 0)
        {
            return kTokenTypes [index].token;
        }
    }

    return {.type = kInvalidNodeType, {.operation = kInvalidFunc}, .line_pos = 0, .number_of_line = 0};
}

#undef CHECK_RESULT
