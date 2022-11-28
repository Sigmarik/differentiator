#include "bin_tree.h"

#include <sys/stat.h>
#include <cstring>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include "util/dbg/debug.h"
#include "file_helper.h"

#include "tree_config.h"

#define equal(alpha, beta) ( abs(alpha - beta) < CMP_EPS )

#define OP_SWITCH_END case OP_NONE:                                                                                     \
        if (err_code) *err_code = EINVAL;                                                                               \
        log_printf(ERROR_REPORTS, "error", "NONE operation detected when differentiating equation %p.\n", equation);    \
        break;                                                                                                          \
    default:                                                                                                            \
        if (err_code) *err_code = EINVAL;                                                                               \
        log_printf(ERROR_REPORTS, "error",                                                                              \
            "Somehow Operation equation->value.op had an incorrect value of %d.\n", equation->value.op);                \
        break;                                                                                                          \

/**
 * @brief Print subtree to the .dot file.
 * 
 * @param equation subtree to print
 * @param file write destination
 * @param err_code variable to use as errno
 */
static void recursive_graph_dump(const Equation* equation, FILE* file, int* const err_code = &errno);

/**
 * @brief Collapse the equation to constant.
 * 
 * @param equation 
 */
static void collapse(Equation* equation);

/**
 * @brief Resolve useless operations, such as multiplication by 1 or 0.
 * 
 * @param equation
 */
static void rm_useless(Equation* equation);

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
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->left->value.op] < OP_PRIORITY[equation->value.op]);

            if (equation->right->type == TYPE_CONST)
                caret_printf(caret, "\\cdot");

            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")",
                equation->right->type == TYPE_OP && OP_PRIORITY[equation->right->value.op] < OP_PRIORITY[equation->value.op]);
            
            break;

        case OP_SIN:
        case OP_COS:
        case OP_LN:
            caret_printf(caret, "\\%s", OP_TEXT_REPS[equation->value.op]);
            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")", true);
            break;
        
        case OP_ADD:
        case OP_SUB:
            surround("(", { Equation_write_as_tex(equation->left, caret); }, ")",
                equation->left->type == TYPE_OP && OP_PRIORITY[equation->left->value.op] < OP_PRIORITY[equation->value.op]);

            caret_printf(caret, "%s", OP_TEXT_REPS[equation->value.op]);

            surround("(", { Equation_write_as_tex(equation->right, caret); }, ")",
                equation->right->type == TYPE_OP && OP_PRIORITY[equation->right->value.op] < OP_PRIORITY[equation->value.op]);
            
            break;
        OP_SWITCH_END
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
#define eq_ln(arg)  eq_op(OP_LN,  eq_const(0),  arg)

Equation* Equation_diff(const Equation* equation, const uintptr_t var_id, int* const err_code) {
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
        case OP_POW: return eq_mul(eq_pow(eq_cL, eq_sub(eq_cR, eq_const(1))),
                                   eq_add( eq_mul(eq_cR, eq_dL),  eq_mul(eq_mul(eq_cL, eq_dR), eq_ln(eq_cL)) ));
        case OP_LN:  return eq_div(eq_dR, eq_cR);
        OP_SWITCH_END
        }
        break;

    default:
        log_printf(ERROR_REPORTS, "error", 
            "Somehow NodeType equation->type had an incorrect value of %d.\n", equation->type);
        break;
    }
    return Equation_copy(equation);
}

#define eq_t_const(eq)  ( eq->type == TYPE_CONST )
#define eq_t_op(eq)     ( eq->type == TYPE_OP )
#define eq_t_var(eq)    ( eq->type == TYPE_VAR )
#define eq_is_val(eq, val)      ( eq->type == TYPE_CONST && equal(eq->value.dbl, val) )
#define eq_is_op(eq, oper)      ( eq->type == TYPE_OP && eq->value.op == oper )
#define eq_is_var(eq, var_id)   ( eq->type == TYPE_VAR && eq->value.id == var_id )
#define eq_L ( equation->left )
#define eq_R ( equation->right )

void Equation_simplify(Equation* equation, int* const err_code) {
    if (!equation) return;
    _LOG_FAIL_CHECK_(!BinaryTree_status(equation), "error", ERROR_REPORTS, return, err_code, EINVAL);

    if (!equation->type == TYPE_OP) return;

    Equation_simplify(eq_L, err_code);
    Equation_simplify(eq_R, err_code);

    collapse(equation);

    rm_useless(equation);
}

double Equation_calculate(const Equation* equation, const double x_value, int* const err_code) {
    if (!equation) return 0.0;

    switch (equation->type) {

    case TYPE_CONST: return equation->value.dbl;
    case TYPE_VAR: return equation->value.id == 'x' ? 0.0 : x_value;

    case TYPE_OP: {
        double alpha = Equation_calculate(equation->left, x_value);
        double beta  = Equation_calculate(equation->right, x_value);

        switch (equation->value.op) {

        case OP_ADD: return alpha + beta;
        case OP_SUB: return alpha - beta;
        case OP_MUL: return alpha * beta;
        case OP_DIV:
            _LOG_FAIL_CHECK_(!equal(beta, 0.0), "error", ERROR_REPORTS, return INFINITY, err_code, EINVAL);
            return alpha / beta;
        case OP_COS: return cos(beta);
        case OP_SIN: return sin(beta);
        case OP_POW: return pow(alpha, beta);
        case OP_LN:  return log(beta);
        OP_SWITCH_END

        }

        break;
    }
    default: break;
    }

    return 0.0;
}

void recursive_graph_dump(const Equation* equation, FILE* file, int* const err_code) {
    _LOG_FAIL_CHECK_(!(BinaryTree_status(equation) & (~TREE_INV_CONNECTIONS)), "error", ERROR_REPORTS, return, err_code, EINVAL);
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

#define eq_value ( equation->value.dbl )
#define get_value(eq) ( eq->value.dbl )
#define as_op(operation) eq_value = get_value(eq_L) operation get_value(eq_R); break

static void collapse(Equation* equation) {
    if (!equation || !eq_t_op(equation)) return;
    if (!eq_t_const(eq_L) || !eq_t_const(eq_R)) return;

    if ( equation->value.op == OP_DIV && ( equal(get_value(eq_R), 0.0) ||
        !equal(get_value(eq_L) / get_value(eq_R), round(get_value(eq_L) / get_value(eq_R))) ) ) return;
    
    if ( equation->value.op == OP_POW && !equal(get_value(eq_R), round(get_value(eq_R))) ) return;
    
    switch (equation->value.op) {
    case OP_ADD: as_op(+);
    case OP_SUB: as_op(-);
    case OP_MUL: as_op(*);
    case OP_DIV: as_op(/);
    case OP_POW:
        eq_value = pow(get_value(eq_L), get_value(eq_R));
        break;
    case OP_LN:
        if (equal(get_value(eq_R), 1.0)) {
            eq_value = 0.0;
            break;
        } else {
            return;
        }
    case OP_SIN:
    case OP_COS:
    case OP_NONE:
    default: return;
    }

    equation->type = TYPE_CONST;

    Equation_dtor(&equation->left);
    Equation_dtor(&equation->right);
}

#define keep_branch(branch) do {            \
    Equation* _branch_copy = branch;        \
    if (eq_R != branch)                     \
        Equation_dtor(&eq_R);               \
    else if (eq_L != branch)                \
        Equation_dtor(&eq_L);               \
    equation->type  = _branch_copy->type;   \
    equation->left  = _branch_copy->left;   \
    equation->right = _branch_copy->right;  \
    equation->value = _branch_copy->value;  \
    free(_branch_copy);                     \
} while (0)

#define keep_const(const_value) do {    \
    equation->type = TYPE_CONST;        \
    Equation_dtor(&equation->left);     \
    Equation_dtor(&equation->right);    \
    equation->value.dbl = (const_value);\
} while (0)

static void rm_useless(Equation* equation) {
    if (!equation || !eq_t_op(equation)) return;
    if (!eq_t_const(eq_L) && !eq_t_const(eq_R)) return;
    
    Equation* const_branch = eq_t_const(eq_L) ? eq_L : eq_R;
    Equation* undef_branch = eq_t_const(eq_L) ? eq_R : eq_L;

    switch (equation->value.op) {
    case OP_ADD:
    case OP_SUB:
        if (equal(const_branch->value.dbl, 0.0))
            keep_branch(undef_branch);
        break;
    case OP_MUL:
        if (equal(const_branch->value.dbl, 1.0))
            keep_branch(undef_branch);
        else if (equal(const_branch->value.dbl, 0.0))
            keep_const(0.0);
        break;
    case OP_DIV:
        if (equal(eq_R->value.dbl, 1.0))
            keep_branch(eq_L);
        else if (equal(eq_L->value.dbl, 0.0))
            keep_const(0.0);
        break;
    case OP_POW:
        if (eq_R->type == TYPE_CONST && equal(eq_R->value.dbl, 1.0))
            keep_branch(eq_L);
        else if (eq_R->type == TYPE_CONST && equal(eq_R->value.dbl, 0.0))
            keep_const(1.0);
        else if (eq_L->type == TYPE_CONST && equal(eq_L->value.dbl, 1.0)) 
            keep_const(1.0);
        else if (eq_L->type == TYPE_CONST && equal(eq_L->value.dbl, 0.0))
            keep_const(0.0);
        break;
    case OP_LN:
    case OP_COS:
    case OP_SIN:
    case OP_NONE:
    default: return;
    }
}
