#include "listqueue.h"
#include <stdint.h>
#include <stdlib.h>

void lq_init(ListQueue *q) { if (q) *q = (ListQueue){0}; }
void lq_destroy(ListQueue *q) {
    if (!q) return;
    while (q->front) {
        QueueNode *next = q->front->next;
        free(q->front);
        q->front = next;
    }
    q->rear = NULL;
    q->size = 0;
}
int lq_enqueue(ListQueue *q, int value) {
    if (!q || q->size == SIZE_MAX) return -1;
    QueueNode *node = malloc(sizeof(*node));
    if (!node) return -1;
    node->value = value;
    node->next = NULL;
    if (q->rear) q->rear->next = node;
    else q->front = node;
    q->rear = node;
    ++q->size;
    return 0;
}
int lq_dequeue(ListQueue *q, int *value) {
    if (!q || !q->front || !value) return -1;
    QueueNode *node = q->front;
    *value = node->value;
    q->front = node->next;
    if (!q->front) q->rear = NULL;
    free(node);
    --q->size;
    return 0;
}
int lq_empty(const ListQueue *q) { return !q || q->front == NULL; }
size_t lq_size(const ListQueue *q) { return q ? q->size : 0; }
