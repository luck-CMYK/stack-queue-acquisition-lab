#include "test.h"
#include "ds/seqstack.h"

static int lifo_and_peek(void) {
    SeqStack s;
    int v;
    CHECK(s_init(&s, 3) == 0 && s.top == -1);
    for (int i = 1; i <= 3; ++i) CHECK(s_push(&s, i) == 0);
    CHECK(s_size(&s) == 3);
    CHECK(s_peek(&s, &v) == 0 && v == 3 && s_size(&s) == 3);
    for (int i = 3; i > 0; --i) { CHECK(s_pop(&s, &v) == 0); CHECK(v == i); }
    CHECK(s_empty(&s) && s.top == -1);
    s_destroy(&s);
    return 0;
}
static int empty_operations(void) {
    SeqStack s;
    int v = 99;
    CHECK(s_init(&s, 1) == 0);
    CHECK(s_pop(&s, &v) == -1 && s_peek(&s, &v) == -1);
    CHECK(v == 99 && s.top == -1);
    s_destroy(&s);
    return 0;
}
static int full_preserves_top(void) {
    SeqStack s;
    int v;
    CHECK(s_init(&s, 2) == 0);
    CHECK(s_push(&s, 7) == 0 && s_push(&s, 8) == 0 && s_full(&s));
    CHECK(s_push(&s, 9) == -1);
    CHECK(s_peek(&s, &v) == 0 && v == 8 && s_size(&s) == 2);
    s_destroy(&s);
    return 0;
}
static int randomized_reference(void) {
    SeqStack s;
    int model[31], count = 0;
    unsigned state = 17U;
    CHECK(s_init(&s, 31) == 0);
    for (int i = 0; i < 100000; ++i) {
        state = state * 1664525U + 1013904223U;
        if ((state >> 24) < 130U) {
            int expected = count == 31 ? -1 : 0;
            CHECK(s_push(&s, i) == expected);
            if (!expected) model[count++] = i;
        } else {
            int v = -1, expected = count ? 0 : -1;
            CHECK(s_pop(&s, &v) == expected);
            if (!expected) CHECK(v == model[--count]);
            else CHECK(v == -1);
        }
        CHECK(s_size(&s) == (size_t)count);
        if (count) { int v; CHECK(s_peek(&s, &v) == 0 && v == model[count - 1]); }
    }
    s_destroy(&s);
    return 0;
}
static int invalid_and_destroy(void) {
    SeqStack s;
    int v;
    CHECK(s_init(NULL, 2) == -1);
    CHECK(s_init(&s, 0) == -1 && s.data == NULL);
    CHECK(s_init(&s, -2) == -1);
    CHECK(s_push(NULL, 1) == -1 && s_pop(NULL, &v) == -1);
    CHECK(s_peek(NULL, &v) == -1);
    CHECK(s_init(&s, 1) == 0);
    CHECK(s_push(&s, 3) == 0);
    CHECK(s_pop(&s, NULL) == -1 && s_peek(&s, NULL) == -1 && s_size(&s) == 1);
    s_destroy(&s); s_destroy(&s); s_destroy(NULL);
    CHECK(s.data == NULL && s.top == -1 && s_empty(&s) && !s_full(&s));
    CHECK(s_push(&s, 4) == -1 && s_pop(&s, &v) == -1);
    CHECK(s_init(&s, 1) == 0);
    s_destroy(&s);
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("LIFO + nondestructive peek", lifo_and_peek, &total, &failed);
    run_test("T5 empty pop/peek", empty_operations, &total, &failed);
    run_test("T6 full push preserves top", full_preserves_top, &total, &failed);
    run_test("100000 operations vs reference LIFO", randomized_reference, &total, &failed);
    run_test("invalid arguments + destroy + reuse", invalid_and_destroy, &total, &failed);
    return test_summary(total, failed);
}
