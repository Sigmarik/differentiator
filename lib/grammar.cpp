#include "grammar.h"

#include <ctype.h>

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
#define SKIP() skip_spaces(caret)

/**
 * @brief Move the caret to the nearest non-space character.
 * 
 * @param caret
 */
static void skip_spaces(const char* *caret);

GRAM(parse) {
    CHECK_INPUT();
    SKIP();
    Equation* value = parse_eq(caret);
    SKIP();
    if (**caret != '\0')
        CERROR("Invalid terminator symbol of 0x%02X detected.\n", (unsigned char)**caret & 0xff);
    return value;
}

GRAM(parse_eq) {
    CHECK_INPUT();
    Equation* value = parse_mult(caret);
    SKIP();
    while (**caret == '+' || **caret == '-') {
        char op = **caret;
        ++*caret;
        SKIP();
        Equation* next_arg = parse_mult(caret);
        value = new_Equation(TYPE_OP, { .op = op=='+' ? OP_ADD : OP_SUB }, value, next_arg);
        SKIP();
    }
    return value;
}

GRAM(parse_mult) {
    CHECK_INPUT();
    Equation* value = parse_pow(caret);
    SKIP();
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
    SKIP();
    while (**caret == '^') {
        ++*caret;
        Equation* next_arg = parse_brackets(caret);
        value = new_Equation(TYPE_OP, { .op = OP_POW }, value, next_arg);
        SKIP();
    }
    return value;
}

GRAM(parse_brackets) {
    CHECK_INPUT();
    Equation* value = NULL;
    SKIP();
    if (**caret == '(') {
        ++*caret;
        value = parse_eq(caret);
        SKIP();
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
    SKIP();
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

static void skip_spaces(const char* *caret) {
    while (isblank(**caret)) ++*caret;
}
