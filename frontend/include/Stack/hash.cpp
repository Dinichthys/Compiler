#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "hash.h"

size_t hashing (const uint8_t* const val, const size_t num)
{
    assert (val != NULL && "Invalid pointer for hashing\n");

    const uint8_t* hash_ptr = val;

    size_t hash_sum = 0;

    while (hash_ptr < val + num)
    {
        hash_sum += (size_t) *hash_ptr;
        hash_ptr++;
    }

    return hash_sum;
}
