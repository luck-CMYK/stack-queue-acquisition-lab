#include "cqueue.h"
#include <stdint.h>
#include <stdlib.h>

static int valid(const CirQueue *q) {
    return q && q->data && q->capacity > 0;
}
int q_init_mode(CirQueue *q, int cap, QueueMode mode) {
    if (!q) return -1;
    *q = (CirQueue){0};
    if ((mode != Q_SPARE_SLOT && mode != Q_COUNTED) ||
        cap < (mode == Q_SPARE_SLOT ? 2 : 1) ||
        (size_t)cap > SIZE_MAX / sizeof(int)) return -1;
    q->data = malloc((size_t)cap * sizeof(*q->data));
    if (!q->data) return -1;
    q->capacity = cap;
    q->mode = mode;
    return 0;
}
int q_init(CirQueue *q, int cap) { return q_init_mode(q, cap, Q_SPARE_SLOT); }
void q_destroy(CirQueue *q) {
    if (q) { free(q->data); *q = (CirQueue){0}; }
}
int q_empty(const CirQueue *q) {
    if (!valid(q)) return 1;
    return q->mode == Q_COUNTED ? q->size == 0 : q->front == q->rear;
}
int q_full(const CirQueue *q) {
    if (!valid(q)) return 0;
    return q->mode == Q_COUNTED ? q->size == q->capacity :
        (q->rear + 1) % q->capacity == q->front;
}
size_t q_size(const CirQueue *q) {
    if (!valid(q)) return 0;
    if (q->mode == Q_COUNTED) return (size_t)q->size;
    return (size_t)(q->rear >= q->front ? q->rear - q->front :
        q->capacity - (q->front - q->rear));
}
size_t q_limit(const CirQueue *q) {
    return valid(q) ? (size_t)(q->capacity - (q->mode == Q_SPARE_SLOT)) : 0;
}
int q_enqueue(CirQueue *q, int value) {
    if (!valid(q) || q_full(q)) return -1;
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    if (q->mode == Q_COUNTED) ++q->size;
    return 0;
}
int q_dequeue(CirQueue *q, int *value) {
    if (!valid(q) || !value || q_empty(q)) return -1;
    *value = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    if (q->mode == Q_COUNTED) --q->size;
    return 0;
}
int q_enqueue_overwrite(CirQueue *q, int value) {
    if (!valid(q)) return -1;
    if (q_full(q)) { int discarded; (void)q_dequeue(q, &discarded); }
    return q_enqueue(q, value);
}
