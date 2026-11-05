#ifndef LISTQUEUE_H
#define LISTQUEUE_H
#include <stddef.h>

typedef struct QueueNode { int value; struct QueueNode *next; } QueueNode;
typedef struct { QueueNode *front, *rear; size_t size; } ListQueue;
void lq_init(ListQueue *q);
void lq_destroy(ListQueue *q);
int lq_enqueue(ListQueue *q, int value);
int lq_dequeue(ListQueue *q, int *value);
int lq_empty(const ListQueue *q);
size_t lq_size(const ListQueue *q);
#endif
