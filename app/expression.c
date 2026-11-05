#include "expression.h"
#include "ds/seqstack.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

static int bounded_length(const char *text, size_t *length) {
    if (!text) return -1;
    size_t n = 0;
    while (n <= EXPR_MAX_LENGTH && text[n]) ++n;
    *length = n;
    return n > EXPR_MAX_LENGTH ? -1 : 0;
}
static int precedence(int op) {
    if (op == 'N' || op == 'P') return 3;
    if (op == '*' || op == '/') return 2;
    return op == '+' || op == '-' ? 1 : 0;
}
static ExprStatus apply(int op, double *values, size_t *count) {
    if (op == 'N' || op == 'P') {
        if (!*count) return EXPR_SYNTAX;
        if (op == 'N') values[*count - 1] = -values[*count - 1];
        return EXPR_OK;
    }
    if (*count < 2) return EXPR_SYNTAX;
    double right = values[--*count], left = values[*count - 1], result;
    switch (op) {
        case '+': result = left + right; break;
        case '-': result = left - right; break;
        case '*': result = left * right; break;
        case '/':
            if (right == 0.0) return EXPR_DIV_ZERO;
            result = left / right;
            break;
        default: return EXPR_SYNTAX;
    }
    if (!isfinite(result)) return EXPR_RANGE;
    values[*count - 1] = result;
    return EXPR_OK;
}

ExprStatus expression_eval(const char *text, double *out, size_t *error_pos) {
    size_t n = 0, p = 0, count = 0;
    ExprStatus status = EXPR_OK;
    SeqStack operators;
    if (error_pos) *error_pos = 0;
    if (!text || !out) return EXPR_ARGUMENT;
    if (bounded_length(text, &n)) return EXPR_TOO_LONG;
    if (s_init(&operators, (int)n + 1)) return EXPR_MEMORY;
    double *values = malloc((n + 1) * sizeof(*values));
    if (!values) { s_destroy(&operators); return EXPR_MEMORY; }
    int need_operand = 1;
    while (p < n) {
        unsigned char c = (unsigned char)text[p];
        if (isspace(c)) { ++p; continue; }
        if (isdigit(c) || c == '.') {
            if (!need_operand) { status = EXPR_SYNTAX; goto done; }
            size_t begin = p, digits = 0;
            while (isdigit((unsigned char)text[p])) { ++p; ++digits; }
            if (text[p] == '.') {
                ++p;
                while (isdigit((unsigned char)text[p])) { ++p; ++digits; }
            }
            if (!digits) { status = EXPR_SYNTAX; goto done; }
            if (text[p] == 'e' || text[p] == 'E') {
                ++p;
                if (text[p] == '+' || text[p] == '-') ++p;
                if (!isdigit((unsigned char)text[p])) { status = EXPR_SYNTAX; goto done; }
                while (isdigit((unsigned char)text[p])) ++p;
            }
            char *end;
            errno = 0;
            double value = strtod(text + begin, &end);
            if (end != text + p) { status = EXPR_SYNTAX; goto done; }
            if (errno == ERANGE || !isfinite(value)) { status = EXPR_RANGE; goto done; }
            values[count++] = value;
            need_operand = 0;
            continue;
        }
        if (c == '(') {
            if (!need_operand) { status = EXPR_SYNTAX; goto done; }
            if (s_push(&operators, '(')) { status = EXPR_MEMORY; goto done; }
            ++p;
            continue;
        }
        if (c == ')') {
            int op, found = 0;
            if (need_operand) { status = EXPR_SYNTAX; goto done; }
            while (s_pop(&operators, &op) == 0) {
                if (op == '(') { found = 1; break; }
                status = apply(op, values, &count);
                if (status != EXPR_OK) goto done;
            }
            if (!found) { status = EXPR_SYNTAX; goto done; }
            ++p;
            continue;
        }
        if (c == '+' || c == '-' || c == '*' || c == '/') {
            int op = (int)c;
            if (need_operand) {
                if (c != '+' && c != '-') { status = EXPR_SYNTAX; goto done; }
                /* Unary signs are right associative; do not reduce them yet. */
                if (s_push(&operators, c == '-' ? 'N' : 'P')) { status = EXPR_MEMORY; goto done; }
                ++p;
                continue;
            }
            int top;
            while (s_peek(&operators, &top) == 0 && top != '(' && precedence(top) >= precedence(op)) {
                (void)s_pop(&operators, &top);
                status = apply(top, values, &count);
                if (status != EXPR_OK) goto done;
            }
            if (s_push(&operators, op)) { status = EXPR_MEMORY; goto done; }
            need_operand = 1;
            ++p;
            continue;
        }
        status = EXPR_SYNTAX;
        goto done;
    }
    if (need_operand) { status = EXPR_SYNTAX; goto done; }
    int op;
    while (s_pop(&operators, &op) == 0) {
        if (op == '(') { status = EXPR_SYNTAX; goto done; }
        status = apply(op, values, &count);
        if (status != EXPR_OK) goto done;
    }
    if (count != 1) { status = EXPR_SYNTAX; goto done; }
    *out = values[0];
done:
    if (error_pos) *error_pos = p;
    free(values);
    s_destroy(&operators);
    return status;
}
const char *expression_status(ExprStatus status) {
    switch (status) {
        case EXPR_OK: return "ok";
        case EXPR_SYNTAX: return "invalid expression syntax";
        case EXPR_DIV_ZERO: return "division by zero";
        case EXPR_RANGE: return "number out of double range";
        case EXPR_TOO_LONG: return "expression exceeds 4096 bytes";
        case EXPR_MEMORY: return "allocation/stack capacity failed";
        case EXPR_ARGUMENT: return "null argument";
        default: return "unknown error";
    }
}
int check_brackets(const char *text) {
    size_t n;
    if (bounded_length(text, &n)) return -1;
    SeqStack stack;
    if (s_init(&stack, (int)n + 1)) return -1;
    int matched = 1;
    for (size_t i = 0; i < n; ++i) {
        int c = (unsigned char)text[i];
        if (c == '(' || c == '[' || c == '{') {
            if (s_push(&stack, c)) { matched = -1; break; }
        } else if (c == ')' || c == ']' || c == '}') {
            int top;
            if (s_pop(&stack, &top) ||
                !((top == '(' && c == ')') || (top == '[' && c == ']') || (top == '{' && c == '}'))) {
                matched = 0;
                break;
            }
        }
    }
    if (matched == 1 && !s_empty(&stack)) matched = 0;
    s_destroy(&stack);
    return matched;
}
