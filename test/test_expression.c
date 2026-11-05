#include "test.h"
#include "app/expression.h"
#include <math.h>
#include <string.h>

static int precedence_and_associativity(void) {
    const char *inputs[] = {"1+2*3", "(1+2)*3", "8/4/2", "10-3-2", " 3.5 + .5 * 2 ", "1e3 / 4", "1/2", "2*(3+(4*5))-6/2"};
    const double expected[] = {7, 9, 1, 5, 4.5, 250, 0.5, 43};
    for (size_t i = 0; i < sizeof(expected)/sizeof(*expected); ++i) {
        double v = -999;
        CHECK(expression_eval(inputs[i], &v, NULL) == EXPR_OK);
        CHECK(fabs(v - expected[i]) < 1e-10);
    }
    return 0;
}
static int unary_operators(void) {
    const char *inputs[] = {"-2*3", "2*-3", "--2", "-(2+3)", "-(-2)", "2/-4", "+-2", "1--2", "2--(3+4)"};
    const double expected[] = {-6, -6, 2, -5, 2, -0.5, -2, 3, 9};
    for (size_t i = 0; i < sizeof(expected)/sizeof(*expected); ++i) {
        double v;
        CHECK(expression_eval(inputs[i], &v, NULL) == EXPR_OK);
        CHECK(fabs(v - expected[i]) < 1e-10);
    }
    return 0;
}
static int syntax_errors(void) {
    const char *inputs[] = {"", " ", "1+", "*2", "()", "(1+2", "1+2)", "2(3)", "1 2", "1..2", "a+1", "1/**2", "[2]", ".", "1e", "1,2", "0x1p2", "2^3"};
    for (size_t i = 0; i < sizeof(inputs)/sizeof(*inputs); ++i) {
        double v = 123;
        size_t pos = 999;
        CHECK(expression_eval(inputs[i], &v, &pos) == EXPR_SYNTAX);
        CHECK(v == 123 && pos <= strlen(inputs[i]));
    }
    return 0;
}
static int arithmetic_errors(void) {
    double v = 42;
    CHECK(expression_eval("1/(2-2)", &v, NULL) == EXPR_DIV_ZERO && v == 42);
    CHECK(expression_eval("1/-0", &v, NULL) == EXPR_DIV_ZERO);
    CHECK(expression_eval("1e309", &v, NULL) == EXPR_RANGE);
    CHECK(expression_eval("1e-999", &v, NULL) == EXPR_RANGE);
    CHECK(expression_eval("1e308*1e308", &v, NULL) == EXPR_RANGE);
    CHECK(expression_eval(NULL, &v, NULL) == EXPR_ARGUMENT);
    CHECK(expression_eval("1", NULL, NULL) == EXPR_ARGUMENT);
    for (int i = EXPR_OK; i <= EXPR_ARGUMENT; ++i) CHECK(strlen(expression_status((ExprStatus)i)) > 0);
    return 0;
}
static int brackets(void) {
    const char *good[] = {"", "abc", "{[()]}", "(a+b)*[c-{d}]"};
    const char *bad[] = {"([)]", "}", "((", "{[()]}]"};
    for (size_t i = 0; i < sizeof(good)/sizeof(*good); ++i) CHECK(check_brackets(good[i]) == 1);
    for (size_t i = 0; i < sizeof(bad)/sizeof(*bad); ++i) CHECK(check_brackets(bad[i]) == 0);
    CHECK(check_brackets(NULL) == -1);
    return 0;
}
static int length_and_deep_nesting(void) {
    char text[EXPR_MAX_LENGTH + 2U];
    double v;
    memset(text, '(', 1500);
    text[1500] = '7';
    memset(text + 1501, ')', 1500);
    text[3001] = '\0';
    CHECK(check_brackets(text) == 1);
    CHECK(expression_eval(text, &v, NULL) == EXPR_OK && v == 7);
    memset(text, ' ', EXPR_MAX_LENGTH);
    text[EXPR_MAX_LENGTH - 1] = '7'; text[EXPR_MAX_LENGTH] = '\0';
    CHECK(expression_eval(text, &v, NULL) == EXPR_OK && v == 7);
    text[EXPR_MAX_LENGTH] = '1'; text[EXPR_MAX_LENGTH + 1U] = '\0';
    CHECK(expression_eval(text, &v, NULL) == EXPR_TOO_LONG);
    CHECK(check_brackets(text) == -1);
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("precedence + left associativity + decimals", precedence_and_associativity, &total, &failed);
    run_test("unary signs + nested expressions", unary_operators, &total, &failed);
    run_test("reject malformed expressions; output unchanged", syntax_errors, &total, &failed);
    run_test("division zero + range + null arguments", arithmetic_errors, &total, &failed);
    run_test("three bracket types", brackets, &total, &failed);
    run_test("length limit + 1500 nested parentheses", length_and_deep_nesting, &total, &failed);
    return test_summary(total, failed);
}
