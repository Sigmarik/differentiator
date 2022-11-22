/**
 * @file grammar.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Text parsing functions.
 * @version 0.1
 * @date 2022-11-22
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "bin_tree.h"

typedef Equation* gram_t(const char* *caret);

gram_t parse;

gram_t parse_eq;

gram_t parse_mult;

gram_t parse_pow;

gram_t parse_brackets;

gram_t parse_number;

#endif