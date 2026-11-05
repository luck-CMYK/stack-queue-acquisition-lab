#include "test.h"
#include "ds/cqueue.h"

static int fifo(void) {
    CirQueue q;
    int v = 0;
    CHECK(q_init(&q, 5) == 0);
    for (int i = 1; i <= 3; ++i) CHECK(q_enqueue(&q, i) == 0);
    CHECK(q_size(&q) == 3);
    for (int i = 1; i <= 3; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
    CHECK(q_empty(&q));
    q_destroy(&q);
    return 0;
}
static int full_and_unchanged(void) {
    CirQueue q;
    CHECK(q_init(&q, 5) == 0);
    CHECK(q_limit(&q) == 4);
    for (int i = 0; i < 4; ++i) CHECK(q_enqueue(&q, i) == 0);
    CHECK(q_full(&q));
    int front = q.front, rear = q.rear;
    CHECK(q_enqueue(&q, 99) == -1);
    CHECK(q.front == front && q.rear == rear && q_size(&q) == 4);
    int v;
    for (int i = 0; i < 4; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
    q_destroy(&q);
    return 0;
}
static int empty_and_unchanged(void) {
    CirQueue q;
    int v = 42;
    CHECK(q_init(&q, 2) == 0);
    int front = q.front, rear = q.rear;
    CHECK(q_dequeue(&q, &v) == -1);
    CHECK(q.front == front && q.rear == rear && v == 42);
    q_destroy(&q);
    return 0;
}
static int wraparound(void) {
    CirQueue q;
    int v;
    CHECK(q_init(&q, 5) == 0);
    for (int i = 1; i <= 4; ++i) CHECK(q_enqueue(&q, i) == 0);
    for (int i = 1; i <= 2; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
    CHECK(q_enqueue(&q, 5) == 0 && q_enqueue(&q, 6) == 0);
    CHECK(q.rear == 1);
    for (int i = 3; i <= 6; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
    q_destroy(&q);
    return 0;
}
static int counted_uses_all_slots(void) {
    CirQueue q;
    int v;
    CHECK(q_init_mode(&q, 4, Q_COUNTED) == 0);
    CHECK(q_limit(&q) == 4);
    for (int i = 0; i < 4; ++i) CHECK(q_enqueue(&q, i) == 0);
    CHECK(q_full(&q) && !q_empty(&q) && q.front == q.rear);
    CHECK(q_enqueue(&q, 4) == -1);
    for (int i = 0; i < 4; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
    CHECK(q_empty(&q) && !q_full(&q));
    q_destroy(&q);
    CHECK(q_init_mode(&q, 1, Q_COUNTED) == 0);
    CHECK(q_enqueue(&q, 7) == 0 && q_full(&q));
    CHECK(q_dequeue(&q, &v) == 0 && v == 7);
    q_destroy(&q);
    return 0;
}
static int equivalent_effective_capacity(void) {
    CirQueue a, b;
    CHECK(q_init_mode(&a, 8, Q_SPARE_SLOT) == 0);
    CHECK(q_init_mode(&b, 7, Q_COUNTED) == 0);
    unsigned state = 20261105U;
    int model[7] = {0}, head = 0, size = 0;
    for (int i = 0; i < 100000; ++i) {
        state = state * 1664525U + 1013904223U;
        if ((state >> 24) < 140U) {
            int expected = size == 7 ? -1 : 0;
            CHECK(q_enqueue(&a, i) == expected && q_enqueue(&b, i) == expected);
            if (!expected) { model[(head + size) % 7] = i; ++size; }
        } else {
            int x = -99, y = -99, expected = size ? 0 : -1;
            CHECK(q_dequeue(&a, &x) == expected && q_dequeue(&b, &y) == expected);
            if (!expected) { CHECK(x == model[head] && y == model[head]); head = (head + 1) % 7; --size; }
            else CHECK(x == -99 && y == -99);
        }
        CHECK(q_size(&a) == (size_t)size && q_size(&b) == (size_t)size);
        CHECK(q_empty(&a) == (size == 0) && q_empty(&b) == (size == 0));
        CHECK(q_full(&a) == (size == 7) && q_full(&b) == (size == 7));
    }
    q_destroy(&a); q_destroy(&b);
    return 0;
}
static int overwrite_oldest(void) {
    for (int mode = 0; mode < 2; ++mode) {
        CirQueue q;
        int v;
        CHECK(q_init_mode(&q, mode ? 3 : 4, (QueueMode)mode) == 0);
        for (int i = 1; i <= 7; ++i) CHECK(q_enqueue_overwrite(&q, i) == 0);
        CHECK(q_full(&q));
        for (int i = 5; i <= 7; ++i) { CHECK(q_dequeue(&q, &v) == 0); CHECK(v == i); }
        q_destroy(&q);
    }
    return 0;
}
static int invalid_and_destroy(void) {
    CirQueue q;
    int v;
    CHECK(q_init(NULL, 2) == -1);
    CHECK(q_init(&q, 0) == -1 && q.data == NULL);
    CHECK(q_init(&q, 1) == -1);
    CHECK(q_init_mode(&q, 2, (QueueMode)9) == -1);
    CHECK(q_enqueue(NULL, 1) == -1 && q_dequeue(NULL, &v) == -1);
    CHECK(q_init(&q, 3) == 0);
    CHECK(q_dequeue(&q, NULL) == -1);
    q_destroy(&q); q_destroy(&q); q_destroy(NULL);
    CHECK(q.data == NULL && q.capacity == 0 && q_size(&q) == 0);
    CHECK(q_enqueue(&q, 1) == -1 && q_dequeue(&q, &v) == -1);
    CHECK(q_init(&q, 3) == 0);
    q_destroy(&q);
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("T1 FIFO", fifo, &total, &failed);
    run_test("T2 full + failed enqueue preserves state", full_and_unchanged, &total, &failed);
    run_test("T3 empty + failed dequeue preserves state", empty_and_unchanged, &total, &failed);
    run_test("T4 wraparound", wraparound, &total, &failed);
    run_test("counted queue uses every slot", counted_uses_all_slots, &total, &failed);
    run_test("100000 operations vs reference FIFO + both modes", equivalent_effective_capacity, &total, &failed);
    run_test("explicit overwrite-oldest policy", overwrite_oldest, &total, &failed);
    run_test("invalid arguments + destroy + reuse", invalid_and_destroy, &total, &failed);
    return test_summary(total, failed);
}
