/**
 * @file artigen.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Article formatter module.
 * @version 0.1
 * @date 2022-11-24
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef ARTIGEN_H
#define ARTIGEN_H

#include "lib/bin_tree.h"

struct Article {
    const char* folder_name = NULL;
    FILE* file = NULL;
    unsigned int max_dif_power = 0;  // maximal derivative power mentioned in the article
};

void Article_ctor(Article* article, const char* dest_folder);
void Article_dtor(Article* article);
bool Article_status(Article* article);

void differentiate(Article* article, const Equation* equation, unsigned int power);

void as_series(Article* article, const Equation* equation, double point, unsigned int power);

void tangent(Article* article, const Equation* equation, double point);

void end(Article* article);

#endif
