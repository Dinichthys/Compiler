#include "language.h"
#include "parse_flags_lang.h"
#include "parse_mode_lang.h"
#include "read_lang.h"
#include "dump_lang.h"
#include "write_tree_lang.h"
#include "write_ir.h"

#include <stdlib.h>
#include <stdio.h>

#include "My_lib/Logger/logging.h"

#define ERROR_HANDLER(error)                                                                            \
    if (error != kDoneLang)                                                                             \
    {                                                                                                   \
        fprintf (stderr, "Code error = {%d} with name \"%s\"\n",                                        \
                         error, EnumErrorToStr (error));                                                \
        TreeDtor (root);                                                                                \
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

    node_t* root = TreeCtor ();

    enum LangError result = kDoneLang;

    result = ReadProgram (set.input_file_name, &root);
    ERROR_HANDLER (result);

    result = DumpLang (root);
    ERROR_HANDLER (result);

    result = WriteDataBase (root);
    ERROR_HANDLER (result);

    result = WriteIR (root, set.stream_out);
    ERROR_HANDLER (result);

    TreeDtor (root);

    SettingsDtor (&set);

    return EXIT_SUCCESS;
}

#undef ERROR_HANDLER
