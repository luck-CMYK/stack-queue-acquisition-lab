#include "test.h"
#include "app/acquisition.h"
#include <limits.h>

typedef struct { int next, calls, fail_every; } Source;
/* A deterministic data source using the public callback interface. */
static int source_read(void *context, int *value) {
    Source *s = context;
    ++s->calls;
    if (s->fail_every && s->calls % s->fail_every == 0) return -1;
    *value = s->next++;
    return 0;
}
static int lifecycle(void) {
    Acquisition a;
    Source source = {10, 0, 0};
    int v;
    CHECK(acq_init(&a, 3, Q_SPARE_SLOT, ACQ_REJECT_NEW, source_read, &source) == 0);
    CHECK(acq_poll(&a) == ACQ_STOPPED && source.calls == 0);
    CHECK(acq_start(&a) == 0 && acq_start(&a) == 0);
    CHECK(acq_poll(&a) == ACQ_OK && acq_pending(&a) == 1);
    acq_stop(&a);
    CHECK(acq_poll(&a) == ACQ_STOPPED && source.calls == 1);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 10);
    CHECK(acq_start(&a) == 0 && acq_poll(&a) == ACQ_OK);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 11);
    acq_destroy(&a); acq_destroy(&a);
    CHECK(acq_start(&a) == -1 && acq_pending(&a) == 0);
    return 0;
}
static int fifo_wrap_and_capacity(void) {
    for (int mode = 0; mode < 2; ++mode) {
        Acquisition a;
        Source source = {1, 0, 0};
        int v;
        CHECK(acq_init(&a, 3, (QueueMode)mode, ACQ_REJECT_NEW, source_read, &source) == 0);
        CHECK(q_limit(&a.buffer) == 3);
        CHECK(acq_start(&a) == 0);
        for (int i = 0; i < 3; ++i) CHECK(acq_poll(&a) == ACQ_OK);
        CHECK(acq_read(&a, &v) == ACQ_OK && v == 1);
        CHECK(acq_read(&a, &v) == ACQ_OK && v == 2);
        CHECK(acq_poll(&a) == ACQ_OK && acq_poll(&a) == ACQ_OK);
        for (int i = 3; i <= 5; ++i) CHECK(acq_read(&a, &v) == ACQ_OK && v == i);
        CHECK(acq_pending(&a) == 0);
        acq_destroy(&a);
    }
    return 0;
}
static int reject_new(void) {
    Acquisition a;
    Source source = {1, 0, 0};
    int v;
    AcqStats stats;
    CHECK(acq_init(&a, 2, Q_COUNTED, ACQ_REJECT_NEW, source_read, &source) == 0);
    CHECK(acq_start(&a) == 0);
    CHECK(acq_poll(&a) == ACQ_OK && acq_poll(&a) == ACQ_OK);
    CHECK(acq_poll(&a) == ACQ_BUFFER_FULL);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 1);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 2);
    CHECK(acq_get_stats(&a, &stats) == 0);
    CHECK(stats.sampled == 3 && stats.consumed == 2 && stats.dropped == 1 && stats.source_errors == 0);
    acq_destroy(&a);
    return 0;
}
static int drop_oldest(void) {
    for (int mode = 0; mode < 2; ++mode) {
        Acquisition a;
        Source source = {1, 0, 0};
        int v;
        AcqStats stats;
        CHECK(acq_init(&a, 2, (QueueMode)mode, ACQ_DROP_OLDEST, source_read, &source) == 0);
        CHECK(acq_start(&a) == 0);
        for (int i = 0; i < 7; ++i) CHECK(acq_poll(&a) == ACQ_OK);
        CHECK(acq_read(&a, &v) == ACQ_OK && v == 6);
        CHECK(acq_read(&a, &v) == ACQ_OK && v == 7);
        CHECK(acq_get_stats(&a, &stats) == 0);
        CHECK(stats.sampled == 7 && stats.consumed == 2 && stats.dropped == 5);
        acq_destroy(&a);
    }
    return 0;
}
static int reader_failure(void) {
    Acquisition a;
    Source source = {100, 0, 2};
    int v;
    AcqStats stats;
    CHECK(acq_init(&a, 2, Q_COUNTED, ACQ_DROP_OLDEST, source_read, &source) == 0);
    CHECK(acq_start(&a) == 0 && acq_poll(&a) == ACQ_OK);
    CHECK(acq_poll(&a) == ACQ_SOURCE_ERROR && acq_pending(&a) == 1);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 100);
    CHECK(acq_poll(&a) == ACQ_OK && acq_read(&a, &v) == ACQ_OK && v == 101);
    CHECK(acq_get_stats(&a, &stats) == 0 && stats.source_errors == 1 && stats.sampled == 2);
    acq_destroy(&a);
    return 0;
}
static int empty_and_null_output(void) {
    Acquisition a;
    Source source = {7, 0, 0};
    int v = 42;
    CHECK(acq_init(&a, 1, Q_SPARE_SLOT, ACQ_REJECT_NEW, source_read, &source) == 0);
    CHECK(acq_read(&a, &v) == ACQ_EMPTY && v == 42);
    CHECK(acq_start(&a) == 0 && acq_poll(&a) == ACQ_OK);
    CHECK(acq_read(&a, NULL) == ACQ_ERROR && acq_pending(&a) == 1);
    CHECK(acq_read(&a, &v) == ACQ_OK && v == 7);
    acq_destroy(&a);
    return 0;
}
static int invalid_arguments_and_reuse(void) {
    Acquisition a;
    Source source = {0};
    int v;
    AcqStats stats;
    CHECK(acq_init(NULL, 2, Q_COUNTED, ACQ_REJECT_NEW, source_read, &source) == -1);
    CHECK(acq_init(&a, 0, Q_COUNTED, ACQ_REJECT_NEW, source_read, &source) == -1);
    CHECK(acq_init(&a, INT_MAX, Q_SPARE_SLOT, ACQ_REJECT_NEW, source_read, &source) == -1);
    CHECK(acq_init(&a, 2, (QueueMode)9, ACQ_REJECT_NEW, source_read, &source) == -1);
    CHECK(acq_init(&a, 2, Q_COUNTED, (AcqPolicy)9, source_read, &source) == -1);
    CHECK(acq_init(&a, 2, Q_COUNTED, ACQ_REJECT_NEW, NULL, NULL) == -1);
    CHECK(acq_read(NULL, &v) == ACQ_ERROR && acq_poll(NULL) == ACQ_ERROR);
    CHECK(acq_get_stats(NULL, &stats) == -1);
    acq_stop(NULL); acq_destroy(NULL);
    CHECK(acq_init(&a, 2, Q_COUNTED, ACQ_REJECT_NEW, source_read, &source) == 0);
    CHECK(acq_get_stats(&a, NULL) == -1);
    CHECK(acq_start(&a) == 0 && acq_poll(&a) == ACQ_OK);
    acq_destroy(&a);
    CHECK(acq_poll(&a) == ACQ_ERROR && acq_read(&a, &v) == ACQ_ERROR);
    CHECK(acq_init(&a, 1, Q_COUNTED, ACQ_REJECT_NEW, source_read, &source) == 0);
    CHECK(acq_get_stats(&a, &stats) == 0 && stats.sampled == 0 && stats.dropped == 0);
    acq_destroy(&a);
    return 0;
}
static int producer_consumer_model(void) {
    for (int mode = 0; mode < 2; ++mode) for (int policy = 0; policy < 2; ++policy) {
        Acquisition a;
        Source source = {0, 0, 17};
        int model[7], head = 0, count = 0;
        uint64_t sampled = 0, consumed = 0, dropped = 0, errors = 0;
        CHECK(acq_init(&a, 7, (QueueMode)mode, (AcqPolicy)policy, source_read, &source) == 0);
        CHECK(acq_start(&a) == 0);
        for (int i = 0; i < 100000; ++i) {
            int next_value = source.next;
            AcqStatus status = acq_poll(&a);
            if (source.calls % 17 == 0) { CHECK(status == ACQ_SOURCE_ERROR); ++errors; }
            else {
                ++sampled;
                if (count == 7) {
                    ++dropped;
                    if (!policy) CHECK(status == ACQ_BUFFER_FULL);
                    else { CHECK(status == ACQ_OK); head = (head + 1) % 7; --count; }
                } else CHECK(status == ACQ_OK);
                if (count < 7) { model[(head + count) % 7] = next_value; ++count; }
            }
            if (i % 5 == 0 && count) {
                int v;
                CHECK(acq_read(&a, &v) == ACQ_OK && v == model[head]);
                head = (head + 1) % 7; --count; ++consumed;
            }
            AcqStats actual;
            CHECK(acq_get_stats(&a, &actual) == 0);
            CHECK(actual.sampled == sampled && actual.consumed == consumed && actual.dropped == dropped && actual.source_errors == errors);
            CHECK(acq_pending(&a) == (size_t)count && sampled == consumed + dropped + (uint64_t)(unsigned int)count);
        }
        acq_stop(&a);
        while (count) { int v; CHECK(acq_read(&a, &v) == ACQ_OK && v == model[head]); head = (head + 1) % 7; --count; }
        acq_destroy(&a);
    }
    return 0;
}
int main(void) {
    int total = 0, failed = 0;
    run_test("A1 start/stop/restart; drain after stop", lifecycle, &total, &failed);
    run_test("A2 sampling FIFO + wrap + equal effective capacity", fifo_wrap_and_capacity, &total, &failed);
    run_test("A3 full buffer rejects newest and counts drop", reject_new, &total, &failed);
    run_test("A4 full buffer retains newest samples", drop_oldest, &total, &failed);
    run_test("A5 source failure preserves queue + recovery", reader_failure, &total, &failed);
    run_test("A6 empty read + null output preserves state", empty_and_null_output, &total, &failed);
    run_test("A7 invalid arguments + destroy + reuse", invalid_arguments_and_reuse, &total, &failed);
    run_test("A8 4 x 100000 producer/consumer reference model", producer_consumer_model, &total, &failed);
    return test_summary(total, failed);
}
