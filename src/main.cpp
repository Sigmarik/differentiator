/**
 * @file main.c
 * @author Ilya Kudryashov (kudriashov.it@phystech.edu)
 * @brief Listworks library showcase.
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

    unsigned int log_threshold = STATUS_REPORTS + 1;
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

    log_printf(STATUS_REPORTS, "status", "Opening file %s as the source database.\n", f_name);

    FILE* source_db = fopen(f_name, "r");
    _LOG_FAIL_CHECK_(source_db, "error", ERROR_REPORTS, {
        log_printf(ERROR_REPORTS, "error", "Failed to open file %s.\n", f_name);

        return_clean(EXIT_FAILURE);

    }, &errno, ENOENT);
    track_allocation(source_db, fclose_void);

    setvbuf(source_db, NULL, _IOFBF, get_file_size(fileno(source_db)));

    BinaryTree decision_tree = {};

    BinaryTree_read(&decision_tree, source_db, &errno);

    track_allocation(decision_tree, BinaryTree_dtor);

    BinaryTree_dump(&decision_tree, STATUS_REPORTS);

    _LOG_FAIL_CHECK_(!BinaryTree_status(&decision_tree), "error", ERROR_REPORTS, return_clean(EXIT_FAILURE), NULL, 0);

    log_printf(STATUS_REPORTS, "status", "Entering main interaction loop.\n");

    say("Here we go.");

    bool running = true;
    while (running) {
        char command = '\0';

        say("What would you like me to do?");

        printf("Command (Q - quit, G - guess, D - definition, C - compare, P - print the graph into logs)\n>>> ");
        scanf(" %c", &command);
        while (getc(stdin) != '\n');
        command = (char)toupper(command);

        log_printf(STATUS_REPORTS, "status", "Encountered command %c.\n", command);

        if (command == 'Q') running = false;
        else execute_command(command, &decision_tree);
        
        _LOG_FAIL_CHECK_(!BinaryTree_status(&decision_tree), "error", ERROR_REPORTS, {
            BinaryTree_dump(&decision_tree, ERROR_REPORTS);
            return_clean(EXIT_FAILURE);
        }, NULL, 0);
    }

    log_printf(STATUS_REPORTS, "status", "Exiting main interaction loop.\n");

    say("How sad. Anyway, do you want me to save what you have done to the database?");

    printf("Save the graph to the same file if was read from?\n>>> ");
    yn_branch({
        log_printf(STATUS_REPORTS, "status", "Saving data to the file %s.\n", f_name);

        FILE* file = fopen(f_name, "w");

        _LOG_FAIL_CHECK_(file, "error", ERROR_REPORTS, {
            puts("Failed to open the file for writing.");
            break;
        }, &errno, ENOENT);

        BinaryTree_write_content(&decision_tree, file, &errno);

    }, {});

    return_clean(errno == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
