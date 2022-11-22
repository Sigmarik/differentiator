#include "bin_tree.h"

#include <sys/stat.h>
#include <cstring>
#include <time.h>
#include <ctype.h>

#include "util/dbg/debug.h"
#include "file_helper.h"

#include "tree_config.h"

#define equal(alpha, beta) ( abs(alpha - beta) < CMP_EPS )

/**
 * @brief Print subtree to the .dot file.
 * 
 * @param equation subtree to print
 * @param file write destination
 * @param err_code variable to use as errno
 */
void recursive_graph_dump(const Equation* equation, FILE* file, int* const err_code = NULL);

Equation* new_Equation(NodeType type, NodeValue value, Equation* left, Equation* right, int* const err_code) {
    Equation* equation = (Equation*) calloc(1, sizeof(*equation));
    _LOG_FAIL_CHECK_(equation, "error", ERROR_REPORTS, return NULL, err_code, ENOMEM);

    equation->type = type;
    equation->value = value;
    equation->left = left;
    equation->right = right;

    return equation;
}

void Equation_dtor(Equation** equation) {
    if (!equation)  return;
    if (!*equation) return;

    if ((*equation)->left)  Equation_dtor(&(*equation)->left);
    if ((*equation)->right) Equation_dtor(&(*equation)->right);
    free(*equation);
    *equation = NULL;
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

#define print_left()  Equation_write_as_input(equation->left,  caret, err_code)
#define print_right() Equation_write_as_input(equation->right, caret, err_code)
#define surround(left, code, right, condition) do { \
    bool __cond = (condition);                      \
    if (__cond) caret_printf(caret, left);          \
    code                                            \
    if (__cond) caret_printf(caret, right);         \
} while (0)

void Equation_write_as_tex(const Equation* equation, caret_t* caret, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return, err_code, EINVAL);
    _LOG_FAIL_CHECK_(caret, "error", ERROR_REPORTS, return, err_code, EINVAL);

    switch (equation->type) {
    case TYPE_VAR:
        caret_printf(caret, "%c", (char)equation->value.id);
        break;

    case TYPE_CONST:
        caret_printf(caret, "%lg", equation->value.dbl);
        break;

    case TYPE_OP: {
        switch (equation->value.op) {
        case OP_POW:
            surround("(", { Equation_write_as_tex(equation->left, caret); }, ")",
                equation->left->type == TYPE_OP);

            caret_printf(caret, "^");

            surround("{", { Equation_write_as_tex(equation->right, caret); }, "}", true);

            break;

        case OP_DIV:
            caret_printf(caret, "\\frac");

            surround("{", { Equation_write_as_tex(equation->left,  caret); }, "}", true);
            surround("{", { Equation_write_as_tex(equation->right, caret); }, "}", true);

            break;

        case OP_MUL:
            surround("(", { Equation_write_as_tex(equation->left, caret); }, ")",
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->left->type] < OP_PRIORITY[OP_MUL]);

            if (!(equation->left->type == TYPE_CONST && equation->right->type == TYPE_VAR))
                caret_printf(caret, "\\cdot");

            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")",
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->right->type] < OP_PRIORITY[OP_MUL]);
            
            break;

        case OP_SIN:
        case OP_COS:
            caret_printf(caret, "\\%s", OP_TEXT_REPS[equation->value.op]);
            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")", true);
            break;
        
        case OP_ADD:
        case OP_SUB:
            surround("(", { Equation_write_as_tex(equation->left, caret); }, ")",
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->left->type] < OP_PRIORITY[OP_MUL]);

            caret_printf(caret, "%s", OP_TEXT_REPS[equation->value.op]);

            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")",
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->right->type] < OP_PRIORITY[OP_MUL]);
            
            break;
        case OP_NONE:
            log_printf(ERROR_REPORTS, "error", "Encountered OP_NONE while processing node %p.\n", equation);
            if (err_code) *err_code = EINVAL;
            break;
        default:
            log_printf(ERROR_REPORTS, "error", "Encountered unknown operation of %d while processing node %p.\n",
                equation->value.op, equation);
            if (err_code) *err_code = EINVAL;
            break;
        }

        break;
    }
    default:
        log_printf(ERROR_REPORTS, "error", 
            "Somehow NodeType equation->type had an incorrect value of %d.\n", equation->type);
        break;
    }
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

Equation* Equation_copy(const Equation* equation) {
    if (!equation) return NULL;
    return new_Equation(equation->type, equation->value, Equation_copy(equation->left), Equation_copy(equation->right));
}

#define eq_cL Equation_copy(equation->left)
#define eq_cR Equation_copy(equation->right)
#define eq_dL Equation_diff(equation->left,  var_id)
#define eq_dR Equation_diff(equation->right, var_id)
#define eq_const(val)                   new_Equation(TYPE_CONST, { .dbl = ( val )       }, NULL, NULL)
#define eq_var(id)                      new_Equation(TYPE_OP,    { .op = OP_ADD         }, NULL, NULL)
#define eq_op(operation, left, right)   new_Equation(TYPE_OP,    { .op = ( operation )  }, left, right)
#define eq_add(left, right) eq_op(OP_ADD, left, right)
#define eq_sub(left, right) eq_op(OP_SUB, left, right)
#define eq_mul(left, right) eq_op(OP_MUL, left, right)
#define eq_div(left, right) eq_op(OP_DIV, left, right)
#define eq_pow(left, right) eq_op(OP_POW, left, right)
#define eq_sin(arg) eq_op(OP_SIN, eq_const(0),  arg)
#define eq_cos(arg) eq_op(OP_COS, eq_const(0),  arg)
#define eq_neg(arg) eq_op(OP_MUL, eq_const(-1), arg)

Equation* Equation_diff(const Equation* equation, const uintptr_t var_id) {
    if (!equation) return NULL;
    switch (equation->type) {
    case TYPE_VAR:
        if (equation->value.id == var_id)
            return eq_const(1);
        else return Equation_copy(equation);
    
    case TYPE_CONST: return eq_const(0);

    case TYPE_OP:
        switch (equation->value.op) {
        case OP_ADD: return eq_add(eq_dL, eq_dR);
        case OP_SUB: return eq_sub(eq_dL, eq_dR);
        case OP_MUL: return eq_add(eq_mul(eq_dL, eq_cR), eq_mul(eq_cL, eq_dR));
        case OP_DIV: return eq_div(eq_sub(eq_mul(eq_dL, eq_cR), eq_mul(eq_cL, eq_dR)), eq_pow(eq_cR, eq_const(2)));
        case OP_SIN: return eq_mul(eq_cos(eq_cR), eq_dR);
        case OP_COS: return eq_mul(eq_neg(eq_sin(eq_cR)), eq_dR);
        case OP_POW:
            log_printf(ERROR_REPORTS, "error", "POW derivatives are not supported yet. Sorry.\n");
            return Equation_copy(equation);
        case OP_NONE:
            log_printf(ERROR_REPORTS, "error", "NONE operation detected when differentiating equation %p.\n", equation);
            return Equation_copy(equation);
        default:
            log_printf(ERROR_REPORTS, "error", 
                "Somehow Operation equation->value.op had an incorrect value of %d.\n", equation->value.op);
            break;
        }
        break;

    default:
        log_printf(ERROR_REPORTS, "error", 
            "Somehow NodeType equation->type had an incorrect value of %d.\n", equation->type);
        break;
    }
    return Equation_copy(equation);
}

#define eq_L simple_l
#define eq_R simple_r
#define eq_same new_Equation(equation->type, equation->value, eq_L, eq_R)
#define eq_operation(operator) do {                                 \
    if (eq_L->type == TYPE_CONST && eq_R->type == TYPE_CONST)       \
        return eq_const(eq_L->value.dbl operator eq_R->value.dbl);  \
    return eq_same;                                                 \
} while (0)

void recursive_graph_dump(const Equation* equation, FILE* file, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return, err_code, EINVAL);
    _LOG_FAIL_CHECK_(file, "error", ERROR_REPORTS, return, err_code, ENOENT);

    if (!equation || !file) return;
    fprintf(file, "\tV%p [shape=\"box\" label=\"", equation);
    switch (equation->type) {
        case TYPE_CONST: fprintf(file, "%lg", equation->value.dbl);             break;
        case TYPE_OP: fprintf(file, "%s", OP_TEXT_REPS[equation->value.op]);    break;
        case TYPE_VAR: fprintf(file, "%c", (char)equation->value.id);           break;
        default:
            log_printf(ERROR_REPORTS, "error", 
                "Somehow NodeType equation->type had an incorrect value of %d.\n", equation->type);
            break;
    }
    fprintf(file, "\"]\n");

    if (equation->left) {
        recursive_graph_dump(equation->left, file);
        fprintf(file, "\tV%p -> V%p [arrowhead=\"none\"]\n", equation, equation->left);
    }
    if (equation->right) {
        recursive_graph_dump(equation->right, file);
        fprintf(file, "\tV%p -> V%p [arrowhead=\"none\"]\n", equation, equation->right);
    }
}
