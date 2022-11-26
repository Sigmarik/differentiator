/**
 * @file tree_config.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Configuration file for tree data structure.
 * @version 0.1
 * @date 2022-11-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TREE_CONFIG_H
#define TREE_CONFIG_H

#include <stdlib.h>

static const size_t TREE_PICT_NAME_SIZE = 256;
static const size_t TREE_DRAW_REQUEST_SIZE = 512;

#define TREE_TEMP_DOT_FNAME "temp.dot"
#define TREE_LOG_ASSET_FOLD_NAME "log_assets"
#define TREE_DUMP_TAG "tree_dump"

static const double CMP_EPS = 1e-7;

enum NodeType {
    TYPE_OP,
    TYPE_VAR,
    TYPE_CONST,
};

enum Operator {
    OP_NONE = 0,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_SIN,
    OP_COS,
    OP_POW,
    OP_LN,
};

//* Operator text representation.
static const char* OP_TEXT_REPS[] = {
    "NONE",
    "+",
    "-",
    "*",
    "/",
    "sin",
    "cos",
    "^",
    "ln"
};

//* Number of operation types.
static const size_t OP_TYPE_COUNT = sizeof(OP_TEXT_REPS) / sizeof(*OP_TEXT_REPS);

//* Operation priority list.
static const int OP_PRIORITY[] = {
    -1, // <-- NONE
    0,  // <-- +
    0,  // <-- -
    1,  // <-- *
    1,  // <-- /
    2,  // <-- sin
    2,  // <-- cos
    3,  // <-- pow (^)
    2,  // <-- ln
};

#endif