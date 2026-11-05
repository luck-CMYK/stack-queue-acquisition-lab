#ifndef SEQSTACK_H
#define SEQSTACK_H
#include <stddef.h>

typedef struct { int *data; int top, cap; } SeqStack;
/* init accepts a new/destroyed object. Destroy before reinitializing live data. */
int s_init(SeqStack *s, int cap);
void s_destroy(SeqStack *s);
int s_push(SeqStack *s, int value);
int s_pop(SeqStack *s, int *value);
int s_peek(const SeqStack *s, int *value);
int s_empty(const SeqStack *s);
int s_full(const SeqStack *s);
size_t s_size(const SeqStack *s);
#endif
