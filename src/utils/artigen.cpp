#include "artigen.h"

#include <string.h>

#include "lib/util/dbg/logger.h"
#include "lib/util/dbg/debug.h"

#include "config.h"

static void put_transition(Article* article);

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

void differentiate(Article* article, const Equation* equation, unsigned int power) {
    Equation* current_stage = Equation_copy(equation);
    unsigned int cur_power = 0;

    while (cur_power < article->max_dif_power && cur_power < power) {
        Equation* next_deriv = Equation_diff(current_stage, 'x');

        log_printf(STATUS_REPORTS, "status", "Differentiation stage:\n");
        Equation_dump(current_stage, STATUS_REPORTS);

        Equation_dtor(&current_stage);
        current_stage = next_deriv;
        Equation_simplify(current_stage);

        ++cur_power;
    }

    static char formula_buffer[MAX_FORMULA_LENGTH] = "";
    memset(formula_buffer, 0, sizeof(formula_buffer));
    caret_t formula_caret = formula_buffer;

    if (cur_power > 0) {
        fill_with(equation);
        fprintf(article->file, "As we have previously discovered that\n\\[(%s)^{(%ld)}=", formula_buffer, (long int)cur_power);

        fill_with(current_stage);
        fprintf(article->file, "%s\\]\n", formula_buffer);

        if (cur_power < power) {
            fprintf(article->file, "Let\'s now continue calculating derivatives.\n");
        }
    }

    while (cur_power < power) {
        put_transition(article);

        fill_with(current_stage);
        fprintf(article->file, "\\[(%s)'=", formula_buffer);

        Equation* next_stage = Equation_diff(current_stage, 'x');
        Equation_dtor(&current_stage);
        current_stage = next_stage;
        Equation_simplify(current_stage);

        log_printf(STATUS_REPORTS, "status", "Differentiation stage:\n");
        Equation_dump(current_stage, STATUS_REPORTS);

        fill_with(current_stage);
        fprintf(article->file, "%s\\]\n", formula_buffer);

        ++cur_power;
    }
}

void as_series(Article* article, const Equation* equation, double point, unsigned int power) {
    SILENCE_UNUSED(article); SILENCE_UNUSED(equation); SILENCE_UNUSED(point); SILENCE_UNUSED(power);
}

void tangent(Article* article, const Equation* equation, double point) {
    SILENCE_UNUSED(article); SILENCE_UNUSED(equation); SILENCE_UNUSED(point);
}

void end(Article* article) {
    SILENCE_UNUSED(article);
}

static void put_transition(Article* article) {
    fprintf(article->file, "%s", TRANSITION_PHRASES[(unsigned int)rand() % TRANSITION_PHRASE_COUNT]);
}