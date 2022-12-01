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

#include <stdlib.h>

#include "util/util.h"

typedef int BinaryTree_status_t;

enum BinaryTreeStatus {
    TREE_NULL               = 1<<0,
    TREE_INV_CONNECTIONS    = 1<<1,
};

static const char* TREE_STATUS_DESCR[] = {
    "Tree pointer was NULL",
    "Tree had invalid connections",
};

static const size_t TREE_REPORT_COUNT = ARR_SIZE(TREE_STATUS_DESCR);

#endif
