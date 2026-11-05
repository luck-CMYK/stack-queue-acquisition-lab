#include "test.h"
#include "ds/liststack.h"

static int lifo_peek_and_empty(void) {
    ListStack s;
    int v = 77;
    ls_init(&s);
    CHECK(ls_empty(&s));
    CHECK(ls_pop(&s, &v) == -1 && ls_peek(&s, &v) == -1 && v == 77);
    CHECK(ls_push(&s, 10) == 0 && ls_push(&s, 20) == 0);
    CHECK(ls_peek(&s, &v) == 0 && v == 20 && ls_size(&s) == 2);
    CHECK(ls_pop(&s, &v) == 0 && v == 20);
    CHECK(ls_pop(&s, &v) == 0 && v == 10 && ls_empty(&s));
    ls_destroy(&s);
    return 0;
}
static int random_and_destroy_nonempty(void) {
    ListStack s;
    int model[64], count = 0;
    unsigned state = 31U;
    ls_init(&s);
    for (int i = 0; i < 50000; ++i) {
        state = state * 1664525U + 1013904223U;
        if (count < 64 && (state >> 24) < 150U) {
            CHECK(ls_push(&s, i) == 0); model[count++] = i;
        } else {
            int v = -1;
            CHECK(ls_pop(&s, &v) == (count ? 0 : -1));
            if (count) CHECK(v == model[--count]);
        }
        CHECK(ls_size(&s) == (size_t)count);
        if (count) { int v; CHECK(ls_peek(&s, &v) == 0 && v == model[count - 1]); }
    }
    CHECK(ls_push(&s, 123) == 0);
    ls_destroy(&s); ls_destroy(&s);
    CHECK(ls_size(&s) == 0 && s.top == NULL);
    return 0;
}
static int invalid_arguments(void) {
    ListStack s;
    int v;
    ls_init(NULL); ls_destroy(NULL); ls_init(&s);
    CHECK(ls_push(NULL, 1) == -1 && ls_pop(NULL, &v) == -1);
    CHECK(ls_peek(NULL, &v) == -1 && ls_empty(NULL));
    CHECK(ls_push(&s, 1) == 0);
    CHECK(ls_pop(&s, NULL) == -1 && ls_peek(&s, NULL) == -1 && ls_size(&s) == 1);
    ls_destroy(&s);
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("linked LIFO + peek + empty", lifo_peek_and_empty, &total, &failed);
    run_test("50000 operations + destroy nonempty", random_and_destroy_nonempty, &total, &failed);
    run_test("linked stack invalid arguments", invalid_arguments, &total, &failed);
    return test_summary(total, failed);
}
