#include "liststack.h"
#include <stdint.h>
#include <stdlib.h>

void ls_init(ListStack *s) { if (s) *s = (ListStack){0}; }
void ls_destroy(ListStack *s) {
    if (!s) return;
    while (s->top) {
        StackNode *next = s->top->next;
        free(s->top);
        s->top = next;
    }
    s->size = 0;
}
int ls_push(ListStack *s, int value) {
    if (!s || s->size == SIZE_MAX) return -1;
    StackNode *node = malloc(sizeof(*node));
    if (!node) return -1;
    node->value = value;
    node->next = s->top;
    s->top = node;
    ++s->size;
    return 0;
}
int ls_pop(ListStack *s, int *value) {
    if (!s || !s->top || !value) return -1;
    StackNode *node = s->top;
    *value = node->value;
    s->top = node->next;
    free(node);
    --s->size;
    return 0;
}
int ls_peek(const ListStack *s, int *value) {
    if (!s || !s->top || !value) return -1;
    *value = s->top->value;
    return 0;
}
int ls_empty(const ListStack *s) { return !s || s->top == NULL; }
size_t ls_size(const ListStack *s) { return s ? s->size : 0; }
