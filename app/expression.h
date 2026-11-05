#ifndef EXPRESSION_H
#define EXPRESSION_H
#include <stddef.h>

#define EXPR_MAX_LENGTH 4096U
typedef enum {
    EXPR_OK = 0, EXPR_SYNTAX, EXPR_DIV_ZERO, EXPR_RANGE,
    EXPR_TOO_LONG, EXPR_MEMORY, EXPR_ARGUMENT
} ExprStatus;

/* Decimal/scientific literals, + - * /, parentheses and unary +/-.
 * out is unchanged on failure. Error position is a zero-based byte offset.
 * This is double arithmetic: 1/2 is 0.5, exact zero divisor is rejected.
 */
ExprStatus expression_eval(const char *text, double *out, size_t *error_pos);
const char *expression_status(ExprStatus status);
/* Returns 1 matched, 0 mismatched, -1 invalid argument/limit/allocation. */
int check_brackets(const char *text);
#endif
