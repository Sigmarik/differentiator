#include "artigen.h"

#include <string.h>
#include <math.h>

#include "lib/util/dbg/logger.h"
#include "lib/util/dbg/debug.h"

#include "config.h"

#define equal(alpha, beta) ( abs(alpha - beta) < CMP_EPS )

#define init_buffer() static char formula_buffer[MAX_FORMULA_LENGTH] = "";  \
    memset(formula_buffer, 0, sizeof(formula_buffer));                      \
    caret_t formula_caret = formula_buffer

/**
 * @brief Print one of the transition phrases into the article.
 * 
 * @param article 
 */
static void put_transition(Article* article);

/**
 * @brief Replace variable under equation with equation derivative.
 * 
 * @param equation 
 */
void to_derivative(Equation** equation);

/**
 * @brief GCD of two numbers.
 * 
 * @param alpha 
 * @param beta 
 * @return greatest common divisor
 */
unsigned long long gcd(unsigned long long alpha, unsigned long long beta);

void Article_ctor(Article* article, const char* dest_folder) {
    _LOG_FAIL_CHECK_(article, "error", ERROR_REPORTS, return, &errno, EFAULT);
    _LOG_FAIL_CHECK_(dest_folder, "error", ERROR_REPORTS, return, &errno, EFAULT);

    static char full_file_name[FILENAME_MAX] = "";
    strncpy(full_file_name, dest_folder, FILENAME_MAX - 1);
    strncat(full_file_name, ART_MAIN_NAME, strlen(full_file_name) - strlen(ART_MAIN_NAME));

    article->folder_name = dest_folder;
    article->file = fopen(full_file_name, "w");

    _LOG_FAIL_CHECK_(article->file, "error", ERROR_REPORTS, return, &errno, ENOENT);
}

void Article_dtor(Article* article) {
    _LOG_FAIL_CHECK_(Article_status(article), "error", ERROR_REPORTS, return, &errno, EINVAL);

    fclose(article->file);
    article->file =  NULL;

    article->folder_name = "";
    article->max_dif_power = 0;
}

bool Article_status(Article* article) {
    if (!article || !article->file) return false;
    return true;
}

#define fill_with(eq) do {                              \
    formula_caret = formula_buffer;                     \
    Equation_write_as_tex(eq, &formula_caret, &errno);  \
    *formula_caret = '\0';                              \
} while (0)

//* Printf for article.
#define put(...) fprintf(article->file, __VA_ARGS__)

void differentiate(Article* article, const Equation* equation, unsigned int power) {
    Equation* current_stage = Equation_copy(equation);
    unsigned int cur_power = 0;

    while (cur_power < article->max_dif_power && cur_power < power) {
        to_derivative(&current_stage);

        ++cur_power;
    }

    init_buffer();

    if (cur_power > 0) {
        fill_with(equation);
        put("We have already proved the statement\n\\[(%s)^{(%ld)}=", formula_buffer, (long int)cur_power);

        fill_with(current_stage);
        put("%s\\]\n", formula_buffer);

        if (cur_power < power) {
            put("Let\'s now continue calculating derivatives.\\newline\n");
        }
    }

    while (cur_power < power) {
        put_transition(article);

        fill_with(current_stage);
        put("\\[(%s)'=", formula_buffer);

        to_derivative(&current_stage);

        fill_with(current_stage);
        put("%s\\]\n", formula_buffer);

        ++cur_power;

        if(current_stage->type == TYPE_CONST) {
            put("As we know, any degree derivative of the constant is equal to "
                "zero, so we can stop differentiating at this point.\\newline\n");
            cur_power = (unsigned int)-1;
        }
    }

    Equation_dtor(&current_stage);

    if (cur_power > article->max_dif_power) article->max_dif_power = cur_power;
}

void as_series(Article* article, const Equation* equation, double point, unsigned int power) {
    init_buffer();

    put("Let's first calculate equation derivatives.\\newline\n");
    differentiate(article, equation, power);

    put("Then we can just place values to previously calculated derivatives and paste them inside the series formula.\n");

    fill_with(equation);
    put("\\[%s=", formula_buffer);

    unsigned long long divisor = 1;
    Equation* current_stage = Equation_copy(equation);
    unsigned int cur_power = 0;
    for (; cur_power < power; divisor *= ++cur_power + 1) {
        to_derivative(&current_stage);

        double value = Equation_calculate(current_stage, 'x');

        if (equal(value, 0.0)) continue;

        double top = value;
        unsigned long long bottom = divisor;

        if (equal(top, round(top))) {
            unsigned long long div = gcd((unsigned long long)abs(top), bottom);
            top /= (double)div;
            bottom /= div;
        }

        if (cur_power) {
            if (bottom != 1) put("+\\frac{%lg}{%lld}", top, (long long)bottom);
            else if (!equal(value, 1.0)) put("%+lg", top);

            if (equal(point, 0.0)) put("x");
            else put("(x%+lg)", -point);

            if (cur_power > 1) put("^{%d}", (int)cur_power);
        } else {
            put("%lg", value);
        }
    }

    if (power > 0) {
        if (!equal(point, 0.0)) put("+o((x%+lg)", -point);
        else put("+o(x");
        if (power > 1) {
            put("^{%d}", (int)power);
        }
        put(")");
    } else {
        put("+o(1)");
    }
    
    Equation_dtor(&current_stage);

    put("\\]");
}

void tangent(Article* article, const Equation* equation, double point) {
    init_buffer();

    double value = Equation_calculate(equation, point);
    Equation* deriv = Equation_diff(equation, 'x');
    double slope_k = Equation_calculate(deriv, point);
    fill_with(equation);
}

static void put_transition(Article* article) {
    put("%s", TRANSITION_PHRASES[(unsigned int)rand() % TRANSITION_PHRASE_COUNT]);
}

void to_derivative(Equation** equation) {
    Equation* next_stage = Equation_diff(*equation, 'x');
    Equation_dtor(equation);
    *equation = next_stage;
    Equation_simplify(*equation);
}

unsigned long long gcd(unsigned long long alpha, unsigned long long beta) {
    while (beta > 0) {
        alpha %= beta;

        alpha ^= beta;
        beta ^= alpha;
        alpha ^= beta;
    }
    return alpha;
}
