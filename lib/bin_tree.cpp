#include "bin_tree.h"

#include <sys/stat.h>
#include <cstring>
#include <time.h>
#include <ctype.h>

#include "util/dbg/debug.h"
#include "file_helper.h"

#include "tree_config.h"

/**
 * @brief Print subtree to the .dot file.
 * 
 * @param equation subtree to print
 * @param file write destination
 * @param err_code variable to use as errno
 */
void recursive_graph_dump(const Equation* equation, FILE* file, int err_code = NULL);

Equation* new_Equation(NodeType type, NodeValue value, Equation* left, Equation* right, int* const err_code) {
    Equation* equation = (Equation*) calloc(1, sizeof(*equation));

    equation->type = type;
    equation->value = value;
    equation->left = left;
    equation->right = right;

    return equation;
}

void Equation_dtor(Equation** equation) {
    if (!equation)  return;
    if (!*equation) return;

    if ((*equation)->left)  Equation_dtor((*equation)->left);
    if ((*equation)->right) Equation_dtor((*equation)->right);
    *equation = NULL;
}

Equation* Equation_read(caret_t* caret, int* const err_code) {
    _LOG_FAIL_CHECK_(caret, "error", ERROR_REPORTS, return, err_code, EINVAL);
    _LOG_FAIL_CHECK_(*caret, "error", ERROR_REPORTS, return, err_code, EINVAL);

    Equation* equation = new_Equation(TYPE_CONST, {.id = 0}, NULL, NULL);

    if ((*caret)[1] != '(') {
        if (isdigit((*caret)[1])) {
            double value = 0;
            int delta = 0;
            sscanf(*caret + 1, "%lf%n", &value, &delta);
            *caret += delta + 2;
                    /*  ^-- length of the " (***) " postfix  */
        } else {
            equation->value.id = tolower((*caret)[1]);
            *caret += 3;
                   /* ^-- length of the " (x) " thingy */
        }
    } else {
        equation->left = Equation_read(caret, err_code);
        equation->type = TYPE_OP;
        switch (**caret) {
            case '+': equation->value.op = OP_ADD; break;
            case '-': equation->value.op = OP_SUB; break;
            case '*': equation->value.op = OP_MUL; break;
            case '/': equation->value.op = OP_DIV; break;
        }
        ++*caret;
        equation->right = Equation_read(caret, err_code);
    }

    return equation;
}

static size_t PictCount = 0;

void _Equation_dump_graph(const Equation* equation, unsigned int importance) {
    BinaryTree_status_t status = BinaryTree_status(equation);
    _log_printf(importance, "tree_dump", "\tEquation at %p (status = %d):\n", equation, status);
    if (status) {
        for (size_t error_id = 0; error_id < TREE_REPORT_COUNT; ++error_id) {
            if (status & (1 << error_id)) {
                _log_printf(importance, "tree_dump", "\t%s\n", TREE_STATUS_DESCR[error_id]);
            }
        }
    }

    if (status & ~TREE_INV_CONNECTIONS) return;

    FILE* temp_file = fopen(TREE_TEMP_DOT_FNAME, "w");
    
    _LOG_FAIL_CHECK_(temp_file, "error", ERROR_REPORTS, return, NULL, 0);

    fputs("digraph G {\n", temp_file);
    fputs(  "\trankdir=TB\n"
            "\tlayout=dot\n"
            , temp_file);

    recursive_graph_dump(equation, temp_file);

    fputc('}', temp_file);
    fclose(temp_file);

    if (system("mkdir -p " TREE_LOG_ASSET_FOLD_NAME)) return;

    time_t raw_time = 0;
    time(&raw_time);

    char pict_name[TREE_PICT_NAME_SIZE] = "";
    sprintf(pict_name, TREE_LOG_ASSET_FOLD_NAME "/pict%04ld_%ld.png", (long int)++PictCount, raw_time);

    char draw_request[TREE_DRAW_REQUEST_SIZE] = "";
    sprintf(draw_request, "dot -Tpng -o %s " TREE_TEMP_DOT_FNAME, pict_name);

    if (system(draw_request)) return;

    _log_printf(importance, "tree_dump",
                "\n<details><summary>Graph</summary><img src=\"%s\"></details>\n", pict_name);
}

void Equation_write_as_input(const Equation* equation, caret_t* caret, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return, err_code, EINVAL);
    _LOG_FAIL_CHECK_(caret, "error", ERROR_REPORTS, return, err_code, EINVAL);

    caret_printf(caret, "(");
    switch (equation->type) {
        case TYPE_VAR:
            caret_printf(caret, "%c", (char)equation->value.id);
            break;
        case TYPE_CONST:
            caret_printf(caret, "%lf", equation->value.dbl);
            break;
        case TYPE_OP:
            Equation_write_as_input(equation->left, caret, err_code);
            caret_printf(caret, "%c", (char)equation->value.op);
            Equation_write_as_input(equation->right, caret, err_code);
            break;
    }
    caret_printf(caret, ")");
}

void Equation_write_as_tex(const Equation* equation, caret_t* caret, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return, err_code, EINVAL);
    _LOG_FAIL_CHECK_(caret, "error", ERROR_REPORTS, return, err_code, EINVAL);

    caret_printf(caret, "(");
    switch (equation->type) {
        case TYPE_VAR:
            caret_printf(caret, "%c", (char)equation->value.id);
            break;
        case TYPE_CONST:
            caret_printf(caret, "%lf", equation->value.dbl);
            break;
        case TYPE_OP:
            Equation_write_as_input(equation->left, caret, err_code);
            caret_printf(caret, "%c", (char)equation->value.op);
            Equation_write_as_input(equation->right, caret, err_code);
            break;
    }
    caret_printf(caret, ")");
}

BinaryTree_status_t BinaryTree_status(const Equation* equation) {
    BinaryTree_status_t status = 0;

    if (equation == NULL) return TREE_NULL;

    if (equation->type == TYPE_OP && (!equation->left || !equation->right)) status |= TREE_INV_CONNECTIONS;
    if (equation->type != TYPE_OP && ( equation->left ||  equation->right)) status |= TREE_INV_CONNECTIONS;

    #ifndef NDEBUG
        if (equation->left)  status |= BinaryTree_status(equation->left);
        if (equation->right) status |= BinaryTree_status(equation->right);
    #endif

    return status;
}

void recursive_graph_dump(const Equation* equation, FILE* file, int err_code) {
    
}
