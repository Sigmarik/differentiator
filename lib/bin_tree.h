/**
 * @file bin_tree.h
 * @author Kudryashov Ilya (kudriashov.it@phystech.edu)
 * @brief Binary tree data structure.
 * @version 0.1
 * @date 2022-11-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef BIN_TREE_H
#define BIN_TREE_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "tree_config.h"
#include "bin_tree_reports.h"
#include "file_helper.h"

enum NodeType {
    TYPE_OP,
    TYPE_VAR,
    TYPE_CONST,
};

enum Operator {
    OP_ADD = '+',
    OP_SUB = '-',
    OP_MUL = '*',
    OP_DIV = '/',
    OP_SIN = 's',
    OP_COS = 'c',
    OP_POW = '^',
};

union NodeValue {
    uintptr_t id;
    Operator op;
    double dbl;
};

struct Equation {
    NodeType type;
    NodeValue value;

    Equation* left = NULL;
    Equation* right = NULL;
};

Equation* new_Equation(NodeType type, NodeValue value, Equation* left, Equation* right, int* const err_code = NULL);
void Equation_dtor(Equation* node);

/**
 * @brief Create binary tree from given data base.
 * 
 * @param equation
 * @param text
 */
size_t Equation_read(Equation* equation, const char* text, int* const err_code = NULL);

/**
 * @brief Dump the list into logs.
 * 
 * @param list
 * @param importance message importance
 */
#define Equation_dump(tree, importance) do {                            \
    log_printf(importance, TREE_DUMP_TAG, "Called list dumping.\n");    \
    _Equation_dump_graph(tree, importance);                             \
} while (0)

/**
 * @brief Put a picture of the tree into logs.
 * 
 * @param tree 
 * @param importance 
 */
void _Equation_dump_graph(const Equation* equation, unsigned int importance);

/**
 * @brief Write equation in input format.
 * 
 * @param tree tree to write to the file
 * @param caret write destination
 * @param err_code variable to use as errno
 */
void Equation_write_as_input(const Equation* equation, caret_t* caret, int* const err_code = NULL);

/**
 * @brief Write equation in tex format.
 * 
 * @param node tree node to write to the file
 * @param caret write destination
 * @param err_code variable to use as errno
 */
void Equation_write_as_tex(const Equation* equation, caret_t* caret, int* const err_code = NULL);

/**
 * @brief Get status of the equation.
 * 
 * @param tree 
 * @return (BinaryTree_status_t) binary tree status (0 = OK)
 */
BinaryTree_status_t BinaryTree_status(const Equation* equation);

#endif
