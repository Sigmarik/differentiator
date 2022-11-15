/**
 * @file bin_tree_reports.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Binary tree report & status values.
 * @version 0.1
 * @date 2022-11-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef BIN_TREE_REPORTS_H
#define BIN_TREE_REPORTS_H

#include "stdlib.h"

typedef int BinaryTree_status_t;

enum BinaryTreeStatus {
    TREE_NULL               = 1<<0,
    TREE_NULL_ROOT          = 1<<2,
    TREE_INV_CONNECTIONS    = 1<<3,
};

static const char* TREE_STATUS_DESCR[] = {
    "Tree pointer was NULL",
    "Tree root was NULL",
    "Tree had invalid connections",
};

static const size_t TREE_REPORT_COUNT = sizeof(TREE_STATUS_DESCR) / sizeof(*TREE_STATUS_DESCR);

#endif
