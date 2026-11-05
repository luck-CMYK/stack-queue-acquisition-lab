#ifndef CQUEUE_H
#define CQUEUE_H

#include <stddef.h>

typedef enum { Q_SPARE_SLOT = 0, Q_COUNTED = 1 } QueueMode;
typedef struct {
    int *data;
    int front, rear, capacity, size;
    QueueMode mode;
} CirQueue;

/* cap is physical array length. Destroy a live object before reinitializing. */
int q_init(CirQueue *q, int cap);
int q_init_mode(CirQueue *q, int cap, QueueMode mode);
void q_destroy(CirQueue *q);
int q_enqueue(CirQueue *q, int value);
int q_dequeue(CirQueue *q, int *value);
int q_empty(const CirQueue *q);
int q_full(const CirQueue *q);
size_t q_size(const CirQueue *q);
size_t q_limit(const CirQueue *q);
/* Explicit sampling policy: replace the oldest item only when full. */
int q_enqueue_overwrite(CirQueue *q, int value);

#endif
