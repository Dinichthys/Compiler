#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "stack.h"

#ifdef HASH_PROT

#include "hash.h"

#endif // HASH_PROT

#ifdef CANARY_PROT

static bool check_canary (stack* const stk);

#endif // CANARY_PROT

static int stack_ok (stack* const stk);
static enum STACK_ERROR stack_resize (stack* const stk, const enum RESIZE_DIRECTION flag);

#ifdef CANARY_PROT

static const size_t CANARY = 1910;

#endif // CANARY_PROT

static const char POISON     = '@';    //  '@' == 64
static const size_t MIN_SIZE = 100;
static const size_t MAX_SIZE = (size_t) -1;     // size_t is unsigned that's why it will make maximum value of stack
static const int STACK_SCALE = 2;
static const size_t KEY      = (size_t) time (NULL);

#ifdef CANARY_PROT

static bool check_canary (stack* const stk)
{
    return (memcmp ((char*) stk - sizeof (CANARY), &CANARY, sizeof (CANARY)) != 0)
        || (memcmp ((char*) stk + sizeof (stack), &CANARY, sizeof (CANARY)) != 0)
        || (memcmp ((char*) (stk->data) - sizeof (CANARY), &CANARY, sizeof (CANARY)) != 0)
        || (memcmp ((char*) (stk->data) + stk->capacity * sizeof (stack_elem), &CANARY, sizeof (CANARY)) != 0);
}

#endif // CANARY_PROT

enum STACK_ERROR stack_ctor (size_t* const stack_encode, const size_t num_elem,
                             const char* file, const int line, const char* func, const char* name)
{
    if (stack_encode == NULL)
    {
        return CANT_CREATE;
    }

    #ifdef CANARY_PROT

    stack* const stk = (stack*) ((char*) calloc (1, sizeof (stack) + 2 * sizeof (CANARY)) + sizeof (CANARY));

    if ((char*) stk - sizeof (CANARY) == NULL)
    {
        return CANT_CREATE;
    }

    memcpy ((char*) stk - sizeof (CANARY), &CANARY, sizeof (CANARY));
    memcpy ((char*) stk + sizeof (stack), &CANARY, sizeof (CANARY));

    #else // CANARY_PROT

    stack* const stk = (stack*) calloc (1, sizeof (stack));

    if (stk == NULL)
    {
        return CANT_CREATE;
    }

    #endif // CANARY_PROT

    *stk = {file, line, func, name, 0, 0, NULL};

    size_t num = (num_elem * sizeof (stack_elem) > MIN_SIZE) ? num_elem : MIN_SIZE;
    num = (num * sizeof (stack_elem) < MAX_SIZE) ? num : MAX_SIZE;

    #ifdef CANARY_PROT

    stk->data = (stack_elem*) ((char*) calloc (num, sizeof (stack_elem) + 2 * sizeof (CANARY)) + sizeof (CANARY));

    if ((char*) stk->data - sizeof (CANARY) == NULL)
    {
        return CANT_CREATE;
    }

    memcpy ((char*) (stk->data) - sizeof (CANARY), &CANARY, sizeof (CANARY));
    memcpy ((char*) (stk->data) + num * sizeof (stack_elem), &CANARY, sizeof (CANARY));

    #else // CANARY_PROT

    stk->data = (stack_elem*) calloc (num, sizeof (stack_elem));

    if (stk->data == NULL)
    {
        return CANT_CREATE;
    }

    #endif // CANARY_PROT

    stk->capacity = num;

    for (size_t i = 0; i < stk->capacity; i++)
    {
        memcpy (stk->data + i, &POISON, sizeof (POISON));
        if (sizeof (stack_elem) - sizeof (POISON) > 0)
        {
            memset ((char*) (stk->data + i) + sizeof (POISON), 0, sizeof (stack_elem) - sizeof (POISON));
        }
    }

    *stack_encode = ((size_t) stk) ^ KEY;

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_CREATE;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    stk->hash_data = hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem));

    stk->hash_stack = hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack));

    #endif // HASH_PROT

    return DONE;
}

enum STACK_ERROR stack_dtor (const size_t stack_encode)
{
    stack* const stk = (stack*) (stack_encode ^ KEY);

    if ((stk  == NULL) || (stack_ok (stk) != DONE))
    {
        return CANT_DESTROY;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_DESTROY;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return CANT_DESTROY;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return CANT_DESTROY;
    }

    #endif // HASH_PROT

    stk->file = NULL;
    stk->line = 0;
    stk->name = NULL;
    stk->size = 0;
    stk->capacity = 0;

    #ifdef CANARY_PROT

    free ((char*) (stk->data) - sizeof (CANARY));

    #else // CANARY_PROT

    free (stk->data);

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    stk->hash_data = 0;
    stk->hash_stack = 0;

    #endif // HASH_PROT

    #ifdef CANARY_PROT

    free ((char*) (stk) - sizeof (CANARY));

    #else // CANARY_PROT

    free (stk);

    #endif // CANARY_PROT

    return DONE;
}

static int stack_ok (stack* const stk)
{
    int error = DONE;

    if (stk == NULL)
    {
        return BAD_STACK;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return BAD_STACK;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return BAD_STACK;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return BAD_STACK;
    }

    #endif // HASH_PROT

    if (stk->file == NULL)
    {
        error |=  BAD_FILE_NAME;
    }

    if (stk->line == 0)
    {
        error |= BAD_LINE_NUM;
    }

    if (stk->name == NULL)
    {
        error |= BAD_NAME;
    }

    if ((stk->size > MAX_SIZE) || (stk->size > stk->capacity))
    {
        error |= BAD_SIZE;
    }

    if ((stk->capacity > MAX_SIZE) || (stk->size > stk->capacity))
    {
        error |= BAD_CAPACITY;
    }

    if (stk->data == NULL)
    {
        error |= BAD_DATA;
    }

    if ((error & BAD_FILE_NAME) && (error & BAD_LINE_NUM) && (error & BAD_NAME)
          && (error & BAD_SIZE) && (error & BAD_CAPACITY) && (error & BAD_DATA))
    {
        error |= BAD_STACK;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return BAD_STACK;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return BAD_STACK;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return BAD_STACK;
    }

    #endif // HASH_PROT

    return error;
}

static enum STACK_ERROR stack_resize (stack* const stk, const enum RESIZE_DIRECTION flag)
{
    if (((flag != UP) && (flag != DOWN)) || (stk == NULL) || (stack_ok (stk) != DONE))
    {
        return CANT_RESIZE;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_RESIZE;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return CANT_RESIZE;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return CANT_RESIZE;
    }

    #endif // HASH_PROT

    if ((flag == UP) && (stk->capacity * sizeof (stack_elem) * STACK_SCALE > MAX_SIZE))
    {
        return CANT_RESIZE;
    }

    size_t previously_capacity = stk->capacity;
    stk->capacity = (flag == UP)
                            ? stk->capacity * STACK_SCALE
                            : stk->capacity / STACK_SCALE;

    #ifdef CANARY_PROT

    stk->data = (stack_elem*) ((char*) realloc ((char*) stk->data - sizeof (CANARY),
                                                stk->capacity * sizeof (stack_elem) + 2 * sizeof (CANARY))
                                                + sizeof (CANARY));

    if ((char*) stk->data - sizeof (CANARY) == NULL)
    {
        return CANT_RESIZE;
    }

    memcpy ((char*) (stk->data) + stk->capacity * sizeof (stack_elem), &CANARY, sizeof (CANARY));

    #else // CANARY_PROT

    stk->data = (stack_elem*) realloc (stk->data, stk->capacity * sizeof (stack_elem));

    if (stk->data == NULL)
    {
        return CANT_RESIZE;
    }

    #endif // CANARY_PROT

    if (flag == UP)
    {
        for (size_t i = previously_capacity; i < stk->capacity; i++)
        {
            memcpy (stk->data + i, &POISON, sizeof (POISON));
            if (sizeof (stack_elem) - sizeof (POISON) > 0)
            {
                memset ((char*) (stk->data + i) + sizeof (POISON), 0, sizeof (stack_elem) - sizeof (POISON));
            }
        }
    }

    #ifdef HASH_PROT

    stk->hash_data = hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem));

    stk->hash_stack = hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack));

    #endif // HASH_PROT

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_RESIZE;
    }

    #endif // CANARY_PROT

    return DONE;
}

enum STACK_ERROR stack_push (const size_t stack_encode, const stack_elem element)
{
    stack* const stk = (stack*) (stack_encode ^ KEY);

    if ((stk == NULL) || (stack_ok (stk) != DONE))
    {
        return CANT_PUSH;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_PUSH;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return CANT_PUSH;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return CANT_PUSH;
    }

    #endif // HASH_PROT

    enum STACK_ERROR error = DONE;

    if (stk->size == stk->capacity)
    {
        error = stack_resize (stk, UP);
        if (error == CANT_RESIZE)
        {
            return CANT_PUSH;
        }
    }

    stk->data [stk->size] = element;
    stk->size++;

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_PUSH;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    stk->hash_data = hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem));

    #endif // HASH_PROT

    return error;
}

enum STACK_ERROR stack_pop (const size_t stack_encode, stack_elem* const element)
{
    stack* const stk = (stack*) (stack_encode ^ KEY);

    if ((stk == NULL) || (element == NULL) || (stack_ok (stk) != DONE))
    {
        return CANT_POP;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_POP;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return CANT_POP;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return CANT_POP;
    }

    #endif // HASH_PROT

    enum STACK_ERROR error = DONE;

    if (stk->size == 0)
    {
        return CANT_POP;
    }

    if (stk->size == stk->capacity / 4)
    {
        error = stack_resize (stk, DOWN);
        if (error == CANT_RESIZE)
        {
            return CANT_PUSH;
        }
    }

    *element = stk->data [stk->size - 1];
    memcpy (stk->data + stk->size - 1, &POISON, sizeof (POISON));
    if (sizeof (stack_elem) - sizeof (POISON) > 0)
    {
        memset ((char*) (stk->data + stk->size - 1) + sizeof (POISON), 0, sizeof (stack_elem) - sizeof (POISON));
    }
    stk->size--;

    #ifdef HASH_PROT

    stk->hash_data = hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem));

    stk->hash_stack = hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack));

    #endif // HASH_PROT

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_POP;
    }

    #endif // CANARY_PROT

    return error;
}

enum STACK_ERROR dump (const size_t stack_encode, const char* const file, const int line)
{
    stack* const stk = (stack*) (stack_encode ^ KEY);

    if ((file == NULL) || (line <= 0))
    {
        return CANT_DUMP;
    }

    if (stk == NULL)
    {
        fprintf (stderr, "\nStruct has NULL pointer.\n");
        return DONE;
    }

    #ifdef CANARY_PROT

    if (check_canary (stk))
    {
        return CANT_DUMP;
    }

    #endif // CANARY_PROT

    #ifdef HASH_PROT

    if (stk->hash_stack != hashing ((const uint8_t*) stk, sizeof (stack) - sizeof (stk->hash_stack)))
    {
        return CANT_DUMP;
    }

    if (stk->hash_data != hashing ((const uint8_t*) stk->data, stk->capacity * sizeof (stack_elem)))
    {
        return CANT_DUMP;
    }

    #endif // HASH_PROT

    fprintf (stderr, "%8.8b\n", (unsigned int) 0); //ANCHOR

    if (stk->name == NULL)
    {
        fprintf (stderr, "\nStruct has invalid pointer for name but has own pointer");
    }
    else
    {
        fprintf (stderr, "\nStruct %s", stk->name);
    }

    fprintf (stderr, " [%p] at %s:%d", stk, file, line);

    if (stk->file == NULL)
    {
        fprintf (stderr, " hasn't name of file where it was born\n");
    }
    else
    {
        fprintf (stderr, " was born at %s", stk->file);
    }

    if (stk->line <= 0)
    {
        fprintf (stderr, " but the number of line where it was born was lost.");
    }
    else
    {
        fprintf (stderr, ":%d", stk->line);
    }

    if (stk->func == NULL)
    {
        fprintf (stderr, " but the function where it was born is undefined\n");
    }
    else
    {
        fprintf (stderr, " (%s)\n", stk->func);
    }

    if (stk->capacity == 0)
    {
        fprintf (stderr, "Struct hasn\'t any elements.\n");
        return DONE;
    }

    if ((stk->size > stk->capacity) || (stk->size > MAX_SIZE))
    {
        fprintf (stderr, "Struct has invalid information about size and capacity.\n");
        return DONE;
    }

    fprintf (stderr, "{\n");

    for (size_t i = 0; i < stk->size; i++)
    {
        fprintf (stderr, "\t#[%lu] ", i);
        size_t j = 0;
        while (j < sizeof (stack_elem))
        {
            unsigned char* symbol = ((unsigned char*) (stk->data) + i * sizeof (stack_elem) + j);
            fprintf (stderr, "%8.8b ", *symbol);
            j++;
        }
        fprintf (stderr, "\n");
    }

    for (size_t i = stk->size; (i < MAX_SIZE) && (i < stk->capacity); i++)
    {
        fprintf (stderr, "\t [%lu] @ POISON\n", i);
    }

    fprintf (stderr, "}\n");

    if (MAX_SIZE < stk->capacity)
    {
        fprintf (stderr, " %lu Stack has much more elements than MAX_SIZE\n", stk->capacity);
    }

    return DONE;
}

const char* decoder_error (const int error)
{
    switch (error)
    {
        CASE_ERROR (DONE);
        CASE_ERROR (CANT_CREATE);
        CASE_ERROR (CANT_DESTROY);
        CASE_ERROR (CANT_RESIZE);
        CASE_ERROR (CANT_PUSH);
        CASE_ERROR (CANT_POP);
        CASE_ERROR (CANT_DUMP);
        default:
            return "Invalid value for error";
    }
}
