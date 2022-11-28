#include "grammar.h"

#include <ctype.h>
#include <cstring>

#include "util/dbg/debug.h"

#define GRAM(name) Equation* name(const char* *caret)
#define CHECK_INPUT() do {                                                      \
    _LOG_FAIL_CHECK_(caret, "error", ERROR_REPORTS, return NULL, NULL, EINVAL); \
    _LOG_FAIL_CHECK_(*caret, "error", ERROR_REPORTS, return NULL, NULL, EINVAL);\
}while (0)
#define CERROR(...) do {                                \
    printf(__VA_ARGS__);                                \
    log_printf(ERROR_REPORTS, "error", __VA_ARGS__);    \
    return value;                                       \
}while (0)

/**
 * @brief Rebuild the line skipping all blank characters.
 * 
 * @param begin 
 */
static void erase_spaces(char* line);

GRAM(parse) {
    CHECK_INPUT();
    char* buffer = (char*) calloc(strlen(*caret) + 1, sizeof(**caret));
    strcpy(buffer, *caret);
    erase_spaces(buffer);
    caret_t buf_caret = buffer;
    log_printf(STATUS_REPORTS, "status", "Equation after space removing procedure: %s\n", buf_caret);
    Equation* value = parse_eq((const char**)&buf_caret);
    if (*buf_caret != '\0') {
        free(buffer);
        CERROR("Invalid terminator symbol of 0x%02X detected.\n", (unsigned char)*buf_caret & 0xff);
    }
    *caret += buf_caret - buffer;

    free(buffer);
    buffer = NULL;
    buf_caret = NULL;

    return value;
}

GRAM(parse_eq) {
    CHECK_INPUT();
    Equation* value = parse_mult(caret);
    while (**caret == '+' || **caret == '-') {
        char op = **caret;
        ++*caret;
        Equation* next_arg = parse_mult(caret);
        value = new_Equation(TYPE_OP, { .op = op=='+' ? OP_ADD : OP_SUB }, value, next_arg);
    }
    return value;
}

GRAM(parse_mult) {
    CHECK_INPUT();
    Equation* value = parse_pow(caret);
    while (**caret == '*' || **caret == '/') {
        char op = **caret;
        ++*caret;
        Equation* next_arg = parse_pow(caret);
        value = new_Equation(TYPE_OP, { .op = op=='*' ? OP_MUL : OP_DIV }, value, next_arg);
    }
    return value;
}

GRAM(parse_pow) {
    CHECK_INPUT();
    Equation* value = parse_brackets(caret);
    
    while (**caret == '^') {
        ++*caret;
        Equation* next_arg = parse_brackets(caret);
        value = new_Equation(TYPE_OP, { .op = OP_POW }, value, next_arg);
    }
    return value;
}

GRAM(parse_brackets) {
    CHECK_INPUT();
    Equation* value = NULL;
    if (**caret == '(') {
        ++*caret;
        value = parse_eq(caret);
        if (**caret != ')') CERROR("Closing bracket expected, got %c instead.\n", **caret);
        ++*caret;
    } else {
        value = parse_number(caret);
    }
    return value;
}

GRAM(parse_number) {
    CHECK_INPUT();
    Equation* value = NULL;
    if (isdigit(**caret)) {
        value = new_Equation(TYPE_CONST, { .dbl = 0.0 }, NULL, NULL);
        int length = 0;
        sscanf(*caret, "%lg%n", &value->value.dbl, &length);
        *caret += length;
    } else if (isalpha(**caret)) {
        value = new_Equation(TYPE_VAR, { .id = (unsigned long)**caret }, NULL, NULL);
        ++*caret;
    }
    return value;
}

static void erase_spaces(char* line) {
    char* left = line;
    for (char* right = line; *right; ++right) {
        if (isblank(*right)) continue;
        *left = *right;
        ++left;
    }
    *left = '\0';
}