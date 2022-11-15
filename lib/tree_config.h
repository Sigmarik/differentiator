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

const size_t TREE_PICT_NAME_SIZE = 256;
const size_t TREE_DRAW_REQUEST_SIZE = 512;

#define TREE_TEMP_DOT_FNAME "temp.dot"
#define TREE_LOG_ASSET_FOLD_NAME "log_assets"
#define TREE_DUMP_TAG "tree_dump"

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
};

static const char* OP_TEXT_REPS[] = {
    "NONE",
    "+",
    "-",
    "*",
    "/",
    "sin",
    "cos",
    "^",
};

static const size_t OP_TYPE_COUNT = sizeof(OP_TEXT_REPS) / sizeof(*OP_TEXT_REPS);

#endif