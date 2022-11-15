/**
 * @file file_helper.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Helper functions for working with files.
 * @version 0.1
 * @date 2022-11-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Jump file reading point to the first mentioned specified character.
 * 
 * @param file file to read from
 * @param target character to search for
 * @param buffer (OPTIONAL) buffer to write skipped characters to
 * @param limit (OPTIONAL) buffer size (0 = infinite)
 * @return int Number of characters that were skipped, -1 in case if char was not found.
 */
int skip_to_char(FILE* file, const char target, char* const buffer = NULL, size_t limit = 0);

/**
 * @brief Safely close the file.
 * 
 * @param ptr Pointer to the file variable.
 */
void fclose_void(FILE** ptr);

/**
 * @brief Get size of the specified file.
 * 
 * @param fd file descriptor
 * @return size_t size of the file
 */
size_t get_file_size(int fd);

typedef char* caret_t;

int caret_printf(caret_t* caret, const char* format, ...) __attribute__((format (printf, 2, 3)));

/**
 * @brief Read whole content of the file and return pointer to the buffer.
 * 
 * @param fname name of the file to open
 * @return char* pointer to the file buffer
 */
char* read_whole(const char* fname);

/**
 * @brief (USE ONLY INSIDE exec_on_char STATEMENT) Stop reading symbols and continue execution.
 * 
 */
#define stop_reading() (__keep_going = false)

/**
 * @brief Execute specific actions on defined characters.
 * 
 * @param file stream to read characters from
 * @param action_list list of cations in the form of @code{.cpp} 
 *          { case 'a': {do_smt(); break;} \ case EOF: stop_reading(); }
 *      @endcode
 */
#define exec_on_char(file, action_list) do {    \
    bool __keep_going = true;                   \
    while (__keep_going) {                      \
        char __in_c = (char)fgetc(file);        \
                                                \
        switch(__in_c) action_list              \
    }                                           \
} while (0)

#endif