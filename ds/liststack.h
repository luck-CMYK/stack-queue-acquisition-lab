#ifndef LISTSTACK_H
#define LISTSTACK_H
#include <stddef.h>

typedef struct StackNode { int value; struct StackNode *next; } StackNode;
typedef struct { StackNode *top; size_t size; } ListStack;
void ls_init(ListStack *s);
void ls_destroy(ListStack *s);
int ls_push(ListStack *s, int value);
int ls_pop(ListStack *s, int *value);
int ls_peek(const ListStack *s, int *value);
int ls_empty(const ListStack *s);
size_t ls_size(const ListStack *s);
#endif
