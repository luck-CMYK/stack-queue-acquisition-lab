#include "test.h"
#include "ds/listqueue.h"

static int fifo_and_last_node(void) {
    ListQueue q;
    int v = 77;
    lq_init(&q);
    CHECK(lq_empty(&q) && lq_dequeue(&q, &v) == -1 && v == 77);
    for (int i = 1; i <= 3; ++i) CHECK(lq_enqueue(&q, i) == 0);
    for (int i = 1; i <= 3; ++i) { CHECK(lq_dequeue(&q, &v) == 0 && v == i); }
    CHECK(q.front == NULL && q.rear == NULL && lq_size(&q) == 0);
    CHECK(lq_enqueue(&q, 42) == 0 && lq_dequeue(&q, &v) == 0 && v == 42);
    lq_destroy(&q);
    return 0;
}
static int random_and_destroy_nonempty(void) {
    ListQueue q;
    int model[64], head = 0, count = 0;
    unsigned state = 51U;
    lq_init(&q);
    for (int i = 0; i < 50000; ++i) {
        state = state * 1664525U + 1013904223U;
        if (count < 64 && (state >> 24) < 150U) {
            CHECK(lq_enqueue(&q, i) == 0); model[(head + count) % 64] = i; ++count;
        } else {
            int v;
            CHECK(lq_dequeue(&q, &v) == (count ? 0 : -1));
            if (count) { CHECK(v == model[head]); head = (head + 1) % 64; --count; }
        }
        CHECK(lq_size(&q) == (size_t)count);
    }
    CHECK(lq_enqueue(&q, 12) == 0);
    lq_destroy(&q); lq_destroy(&q);
    CHECK(q.front == NULL && q.rear == NULL && lq_empty(&q));
    return 0;
}
static int invalid_arguments(void) {
    ListQueue q;
    int v;
    lq_init(NULL); lq_destroy(NULL); lq_init(&q);
    CHECK(lq_enqueue(NULL, 1) == -1 && lq_dequeue(NULL, &v) == -1);
    CHECK(lq_enqueue(&q, 1) == 0);
    CHECK(lq_dequeue(&q, NULL) == -1 && lq_size(&q) == 1);
    lq_destroy(&q);
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("linked FIFO + last-node reset", fifo_and_last_node, &total, &failed);
    run_test("50000 operations + destroy nonempty", random_and_destroy_nonempty, &total, &failed);
    run_test("linked queue invalid arguments", invalid_arguments, &total, &failed);
    return test_summary(total, failed);
}
