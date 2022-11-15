#include "main_utils.h"

#include <stdlib.h>
#include <stdarg.h>

#include "lib/util/dbg/logger.h"
#include "lib/util/dbg/debug.h"

#include "lib/speaker.h"

/**
 * @brief Print one parameter of the object in the form of "is (not) an object(, )"
 * 
 * @param destination the string where to print the argument.
 * @param node argument to print
 * @param next_node nex node in definition path
 * @param is_last is the curent node the last one in the list
 */
static size_t print_argument(char* destination, const TreeNode* node, const TreeNode* next_node, bool is_last);

void MemorySegment_ctor(MemorySegment* segment) {
    segment->content = (int*) calloc(segment->size, sizeof(*segment->content));
}

void MemorySegment_dtor(MemorySegment* segment) {
    free(segment->content);
    segment->content = NULL;
    segment->size = 0;
}

void _MemorySegment_dump(MemorySegment* segment, unsigned int importance) {
    for (size_t id = 0; id < segment->size; ++id) {
        _log_printf(importance, "dump", "[%6lld] = %d\n", (long long) id, segment->content[id]);
    }
}

int clamp(const int value, const int left, const int right) {
    if (value < left) return left;
    if (value > right) return right;
    return value;
}

// Amazing, do not change anything!
// Completed the owl, sorry.
void print_owl(const int argc, void** argv, const char* argument) {
    SILENCE_UNUSED(argc); SILENCE_UNUSED(argv); SILENCE_UNUSED(argument);
    printf("-Owl argument detected, dropping emergency supply of owls.\n");
    for (int index = 0; index < NUMBER_OF_OWLS; index++) {
        puts(R"(    A_,,,_A    )");
        puts(R"(   ((O)V(O))   )");
        puts(R"(  ("\"|"|"/")  )");
        puts(R"(   \"|"|"|"/   )");
        puts(R"(     "| |"     )");
        puts(R"(      ^ ^      )");
    }
}

void mute_speaker(const int argc, void** argv, const char* argument) {
    SILENCE_UNUSED(argc); SILENCE_UNUSED(argv); SILENCE_UNUSED(argument);
    speaker_set_mute(false);
}

void print_label() {
    printf("Guesser game by Ilya Kudryashov.\n");
    printf("Program uses binary tree to guess things.\n");
    printf("Build from\n%s %s\n", __DATE__, __TIME__);
    log_printf(ABSOLUTE_IMPORTANCE, "build info", "Build from %s %s.\n", __DATE__, __TIME__);
}

const char* get_input_file_name(const int argc, const char** argv) {
    const char* file_name = NULL;

    for (int argument_id = 1; argument_id < argc; ++argument_id) {
        if (*argv[argument_id] == '-') continue;
        file_name = argv[argument_id];
        break;
    }

    return file_name;
}

const char* get_output_file_name(const int argc, const char** argv) {
    const char* file_name = NULL;

    bool enc_first_name = false;
    for (int argument_id = 1; argument_id < argc; ++argument_id) {
        if (*argv[argument_id] == '-') continue;
        file_name = argv[argument_id];
        if (enc_first_name) return file_name;
        else enc_first_name = true;
    }

    return NULL;
}

void execute_command(char cmd, BinaryTree* tree, int* const err_code) {
    switch(cmd) {
    case 'G': {
        guess(tree, err_code);
        break;
    }
    case 'D': {
        say("What do you need to know more about?");

        printf("Which word do you want me to give definition of?\n>>> ");
        char word[MAX_INPUT_LENGTH] = "";
        fgets(word, MAX_INPUT_LENGTH - 1, stdin);
        word[strlen(word) - 1] = '\0';
        define(tree, word, err_code);
        break;
    }
    case 'P': {
        say("Trust me, I passed this task over to my friend.");

        log_printf(ABSOLUTE_IMPORTANCE, "dump_info", "Called dump on user request.\n");
        BinaryTree_dump(tree, ABSOLUTE_IMPORTANCE);
        break;
    }
    case 'C': {
        say("What is the first thingy you want me to compare?");

        printf("What is the first thing to compare?\n>>> ");

        char word_a[MAX_INPUT_LENGTH] = "";
        fgets(word_a, MAX_INPUT_LENGTH - 1, stdin);
        word_a[strlen(word_a) - 1] = '\0';

        say("And what do you want to compare %s to?", word_a);

        printf("What to compare %s to?\n>>> ", word_a);

        char word_b[MAX_INPUT_LENGTH] = "";
        fgets(word_b, MAX_INPUT_LENGTH - 1, stdin);
        word_b[strlen(word_b) - 1] = '\0';

        compare(tree, word_a, word_b, err_code);
        break;
    }
    default: {
        say("This task is too easy. Try something harder.");

        printf("Incorrect command, enter command from the list.\n");
        break;
    }
    }
}

void guess(BinaryTree* tree, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(tree), "error", ERROR_REPORTS, return, err_code, EFAULT);

    log_printf(STATUS_REPORTS, "status", "Starting guessing...\n");

    TreeNode* node = tree->root;

    while (node->left && node->right) {
        say("Is it %s?", node->value);

        printf("Is it %s? (yes/no)\n>>> ", node->value);
        log_printf(STATUS_REPORTS, "status", "Made a suggestion of \"%s\".\n", node->value);
        yn_branch({ node = node->left; }, { node = node->right; });
    }

    log_printf(STATUS_REPORTS, "errno", "Suggested answer: \"%s\".\n",node->value);

    say("It has to be %s. Am I right?", node->value);

    printf("It must be %s. Is it? (yes/no)\n>>> ", node->value);
    yn_branch({
        log_printf(STATUS_REPORTS, "errno", "Word \"%s\" was guessed correctly.\n",node->value);
        puts("Yay!");
        say("How boring.");
    }, /* else */ {
        char new_name[MAX_INPUT_LENGTH] = "";

        say("You must have been mistaking. Are you sure that you are right? "
            "If so, enter what you thought was the correct answer below.");

        printf("What was the correct answer?\n>>> ");

        fgets(new_name, MAX_INPUT_LENGTH, stdin);

        char* value_buffer = (char*) calloc(strnlen(new_name, MAX_INPUT_LENGTH) + 1, sizeof(*value_buffer));
        _LOG_FAIL_CHECK_(value_buffer, "error", ERROR_REPORTS, return, err_code, ENOMEM);
        memcpy(value_buffer, new_name, strnlen(new_name, MAX_INPUT_LENGTH) - 1);

        log_printf(STATUS_REPORTS, "status", "Correct answer according to the user: \"%s\".\n", new_name);

        if (BinaryTree_find(tree, value_buffer)) {
            log_printf(STATUS_REPORTS, "status", "New word \"%s\" was already defined. Insertion aborted.\n", value_buffer);
            say("Nah, word %s has another meaning. You are wrong!", value_buffer);
            printf("Word %s already exists.\n", value_buffer);
        }

        TreeNode* alpha_node = (TreeNode*) calloc(1, sizeof(*alpha_node));
        TreeNode_ctor(alpha_node, value_buffer, true, node, false, err_code);

        TreeNode* beta_node = (TreeNode*) calloc(1, sizeof(*beta_node));
        TreeNode_ctor(beta_node, node->value, node->free_value, node, true, err_code);

        say("What is the difference between %s and %s?", value_buffer, node->value);

        printf("What is %s that %s is not?\nIt is ", value_buffer, node->value);

        char new_question[MAX_INPUT_LENGTH] = "";
        fgets(new_question, MAX_INPUT_LENGTH, stdin);

        char* criteria = (char*) calloc(strnlen(new_question, MAX_INPUT_LENGTH) + 1, sizeof(*criteria));
        _LOG_FAIL_CHECK_(criteria, "error", ERROR_REPORTS, return, err_code, ENOMEM);
        memcpy(criteria, new_question, strnlen(new_question, MAX_INPUT_LENGTH) - 1);

        log_printf(STATUS_REPORTS, "status", "Suggested criteria of selection between \"%s\" (as YES) and \"%s\" (as NO) is \"%s\".\n",
                value_buffer, node->value, criteria);

        node->value = criteria;
        node->free_value = true;
    });
}

void define(BinaryTree* tree, const char* word, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(tree), "error", ERROR_REPORTS, return, err_code, EFAULT);
    _LOG_FAIL_CHECK_(word, "error", ERROR_REPORTS, return, err_code, EFAULT);

    log_printf(STATUS_REPORTS, "status", "Asked for the definition of the word %s.\n", word);

    const TreeNode* node = BinaryTree_find(tree, word, err_code);

    if (!node) {

        log_printf(STATUS_REPORTS, "errno", "Word \"%s\" was not found.\n", word);
        say("You must have made a mistake spelling this word. It does not exist.");
        printf("Word was not found!\n");

    } else if (node == tree->root) {

        log_printf(STATUS_REPORTS, "errno", "Word \"%s\" was the only word in the tree.\n", word);
        say("It is the only word humanity knows about at this point in time.");
        printf("It is the only known word...\n");

    } else {

        char phrase[MAX_PHRASE_LENGTH] = "";
        char* phrase_typer = phrase;

        phrase_typer += sprintf(phrase_typer, "It ");

        const TreeNode* chain[MAX_TREE_DEPTH] = {};
        size_t depth = 0;

        BinaryTree_fill_path(node, chain, &depth, MAX_TREE_DEPTH, err_code);

        --depth; // <- Account for the answer node at the end of each path.

        for (size_t index = 0; index < depth; ++index) {
            phrase_typer += print_argument(phrase_typer, chain[index], chain[index + 1], index == depth - 1);
        }

        phrase_typer += sprintf(phrase_typer, ".\n");

        log_printf(STATUS_REPORTS, "status", "Assembled definition - \"%s\".\n", phrase);

        printf("%s", phrase);
        say("%s", phrase);
    }
}

void compare(BinaryTree* tree, const char* word_a, const char* word_b, int* const err_code) {
    _LOG_FAIL_CHECK_(!BinaryTree_status(tree),   "error", ERROR_REPORTS, return, err_code, EFAULT);
    _LOG_FAIL_CHECK_(word_a, "error", ERROR_REPORTS, return, err_code, EFAULT);
    _LOG_FAIL_CHECK_(word_b, "error", ERROR_REPORTS, return, err_code, EFAULT);

    log_printf(STATUS_REPORTS, "status", "Asked for the comparison of words \"%s\", \"%s\".\n", word_a, word_b);

    const TreeNode* node_a = BinaryTree_find(tree, word_a, err_code);
    const TreeNode* node_b = BinaryTree_find(tree, word_b, err_code);

    if (node_a == NULL || node_b == NULL) {

        log_printf(STATUS_REPORTS, "errno", "One of the words was not found.\n");
        say("One of the words is unknown to mankind. You must have made a mistake.");
        puts("One of the words was not found.");

        return;

    }

    if (node_a == node_b) {

        log_printf(STATUS_REPORTS, "errno", "They were the same word.\n");
        say("They are the same object. Or at least the title says so.");
        puts("They are the same objects...");

        return;

    }

    // TODO: Can anyone see what is the problem here?
    // TODO: Can anyone see what is the problem here?
    const TreeNode* path_a[MAX_TREE_DEPTH] = {};
    const TreeNode* path_b[MAX_TREE_DEPTH] = {};
    size_t depth_a = 0;
    size_t depth_b = 0;
    BinaryTree_fill_path(node_a, path_a, &depth_a, MAX_TREE_DEPTH, err_code);
    BinaryTree_fill_path(node_b, path_b, &depth_b, MAX_TREE_DEPTH, err_code);

    size_t prefix_length = 0;

    for (size_t index = 1; index < depth_a && index < depth_b; ++index) {
        if (path_a[index] == path_b[index]) continue;
        prefix_length = index - 1;
        break;
    }

    log_printf(STATUS_REPORTS, "errno", "Found %lld criteria matches.\n", (long long)prefix_length);

    char phrase[MAX_PHRASE_LENGTH] = "";
    char* phrase_typer = phrase;
    
    if (prefix_length == 0) {
        phrase_typer += sprintf(phrase_typer, "These objects have nothing in common, as\n");
    } else {
        phrase_typer += sprintf(phrase_typer, "These objects are similar to each other as they both can be described as \'");
        for (size_t index = 0; index < prefix_length; ++index) {
            phrase_typer += print_argument(phrase_typer, path_a[index], path_a[index + 1], index == prefix_length - 1);
        }
        phrase_typer += sprintf(phrase_typer, "\', while\n");
    }

    --depth_a;
    --depth_b;

    phrase_typer += sprintf(phrase_typer, "object %s ", word_a);
    for (size_t index = prefix_length; index < depth_a; ++index) {
        phrase_typer += print_argument(phrase_typer, path_a[index], path_a[index + 1], index == depth_a - 1);
    }

    phrase_typer += sprintf(phrase_typer, ", and\nobject %s ", word_b);
    for (size_t index = prefix_length; index < depth_b; ++index) {
        phrase_typer += print_argument(phrase_typer, path_b[index], path_b[index + 1], index == depth_b - 1);
    }
    phrase_typer += sprintf(phrase_typer, ".\n");

    log_printf(STATUS_REPORTS, "status", "Assembled phrase - \"%s\".\n", phrase);

    printf("%s", phrase);
    say("%s", phrase);
}

static size_t print_argument(char* destination, const TreeNode* node, const TreeNode* next_node, bool is_last) {
    return (size_t)sprintf(destination, "is%s %s%s", 
            node->right == next_node ? " not" : "",
            node->value, is_last ? "" : ", ");
}
