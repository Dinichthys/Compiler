#include "../language.h"
#include "flags/include/parse_flags_lang.h"
#include "flags/include/parse_mode_lang.h"

#include "My_lib/Assert/my_assert.h"
#include "My_lib/Logger/logging.h"

enum LangError ParseMode (const settings_of_program_t* const set, node_t** const root)
{
    ASSERT (set  != NULL, "Invalid argument set = %p\n",  set);
    ASSERT (root != NULL, "Invalid argument root = %p\n", root);

    switch (set->mode)
    {
        case kNoMode:
        {
            return kDoneLang;
        }
        case kInvalidModeType:
        default:
        {
            return kInvalidModeTypeLangError;
        }
    }
}
