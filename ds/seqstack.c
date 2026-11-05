#include "seqstack.h"
#include <stdint.h>
#include <stdlib.h>

int s_init(SeqStack *s, int cap) {
    if (!s) return -1;
    *s = (SeqStack){NULL, -1, 0};
    if (cap < 1 || (size_t)cap > SIZE_MAX / sizeof(int)) return -1;
    s->data = malloc((size_t)cap * sizeof(*s->data));
    if (!s->data) return -1;
    s->cap = cap;
    return 0;
}
void s_destroy(SeqStack *s) {
    if (s) { free(s->data); *s = (SeqStack){NULL, -1, 0}; }
}
int s_empty(const SeqStack *s) { return !s || !s->data || s->top == -1; }
int s_full(const SeqStack *s) { return s && s->data && s->top == s->cap - 1; }
size_t s_size(const SeqStack *s) { return s && s->data ? (size_t)(s->top + 1) : 0; }
int s_push(SeqStack *s, int value) {
    if (!s || !s->data || s_full(s)) return -1;
    s->data[++s->top] = value;
    return 0;
}
int s_pop(SeqStack *s, int *value) {
    if (!value || s_empty(s)) return -1;
    *value = s->data[s->top--];
    return 0;
}
int s_peek(const SeqStack *s, int *value) {
    if (!value || s_empty(s)) return -1;
    *value = s->data[s->top];
    return 0;
}
