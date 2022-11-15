/**
 * @file main.c
 * @author Ilya Kudryashov (kudriashov.it@phystech.edu)
 * @brief Equation differentiator.
 * @version 0.1
 * @date 2022-08-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ctype.h>

#include "lib/util/dbg/debug.h"
#include "lib/util/argparser.h"
#include "lib/alloc_tracker/alloc_tracker.h"
#include "lib/file_helper.h"
#include "lib/speaker.h"

#include "utils/config.h"

#include "lib/bin_tree.h"

#include "utils/main_utils.h"

#define MAIN

int main(const int argc, const char** argv) {
    atexit(log_end_program);

    start_local_tracking();

    unsigned int log_threshold = STATUS_REPORTS;
    void* log_threshold_wrapper[] = { &log_threshold };

    ActionTag line_tags[] = {
        #include "cmd_flags/main_flags.h"
    };
    const int number_of_tags = sizeof(line_tags) / sizeof(*line_tags);

    parse_args(argc, argv, number_of_tags, line_tags);
    log_init("program_log.html", log_threshold, &errno);
    print_label();

    const char* f_name = DEFAULT_DB_NAME;
    const char* suggested_name = get_input_file_name(argc, argv);
    if (suggested_name) f_name = suggested_name;

    log_printf(STATUS_REPORTS, "status", "Opening file %s as the equation source.\n", f_name);

    char* source_equation = read_whole(f_name);
    caret_t source_caret = source_equation;

    Equation* equation = Equation_read(&source_caret, &errno);
    track_allocation(equation, Equation_dtor);

    free(source_equation);

    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return_clean(EXIT_FAILURE), NULL, 0);

    Equation_dump(equation, ABSOLUTE_IMPORTANCE);

    return_clean(errno == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
