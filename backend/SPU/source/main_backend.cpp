#include <stdlib.h>
#include <stdio.h>

#include "language.h"
#include "parse_flags_lang.h"
#include "struct_lang.h"
#include "translationSPU.h"

#include "My_lib/Logger/logging.h"

#define ERROR_HANDLER(error)                                                                            \
    if (error != kDoneLang)                                                                             \
    {                                                                                                   \
        fprintf (stderr, "Code error = {%d} with name \"%s\"\n",                                        \
                         error, EnumErrorToStr (error));                                                \
        SettingsDtor (&set);                                                                            \
        return EXIT_FAILURE;                                                                            \
    }

int main (const int argc, char* argv[])
{
    settings_of_program_t set = {};
    SettingsCtor (&set);
    ParseFlags (argc, argv, &set);
    if (set.stop_program)
    {
        SettingsDtor (&set);
        return EXIT_FAILURE;
    }

    set_log_file (set.stream_err);
    set_log_lvl (kDebug);

    enum LangError result = kDoneLang;

    result = GenerateAsmSPUFromIR (set.stream_in, set.stream_out);
    ERROR_HANDLER (result);

    SettingsDtor (&set);

    return EXIT_SUCCESS;
}

#undef ERROR_HANDLER
