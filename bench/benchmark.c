#include "ds/cqueue.h"
#include "ds/seqstack.h"
#include "ds/liststack.h"
#include "ds/listqueue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
static LARGE_INTEGER frequency;
static double now_ms(void) {
    LARGE_INTEGER counter;
    (void)QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
}
#else
#include <time.h>
static double now_ms(void) {
    struct timespec t;
    (void)timespec_get(&t, TIME_UTC);
    return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1000000.0;
}
#endif

enum { PAIRS = 100000, LIMIT = 64, ROUNDS = 5 };
typedef union { CirQueue q; SeqStack s; ListStack ls; ListQueue lq; } Storage;
typedef struct { int insert, value; } Operation;
typedef struct {
    const char *name;
    int (*init)(Storage *);
    int (*push)(Storage *, int);
    int (*pop)(Storage *, int *);
    void (*destroy)(Storage *);
} Adapter;
static int init_spare(Storage *s) { return q_init_mode(&s->q, LIMIT + 1, Q_SPARE_SLOT); }
static int init_count(Storage *s) { return q_init_mode(&s->q, LIMIT, Q_COUNTED); }
static int push_q(Storage *s, int v) { return q_enqueue(&s->q, v); }
static int pop_q(Storage *s, int *v) { return q_dequeue(&s->q, v); }
static void destroy_q(Storage *s) { q_destroy(&s->q); }
static int init_seq(Storage *s) { return s_init(&s->s, LIMIT); }
static int push_s(Storage *s, int v) { return s_push(&s->s, v); }
static int pop_s(Storage *s, int *v) { return s_pop(&s->s, v); }
static void destroy_s(Storage *s) { s_destroy(&s->s); }
static int init_ls(Storage *s) { ls_init(&s->ls); return 0; }
static int push_ls(Storage *s, int v) { return ls_push(&s->ls, v); }
static int pop_ls(Storage *s, int *v) { return ls_pop(&s->ls, v); }
static void destroy_ls(Storage *s) { ls_destroy(&s->ls); }
static int init_lq(Storage *s) { lq_init(&s->lq); return 0; }
static int push_lq(Storage *s, int v) { return lq_enqueue(&s->lq, v); }
static int pop_lq(Storage *s, int *v) { return lq_dequeue(&s->lq, v); }
static void destroy_lq(Storage *s) { lq_destroy(&s->lq); }
static const Adapter adapters[] = {
    {"cqueue_spare", init_spare, push_q, pop_q, destroy_q},
    {"cqueue_counted", init_count, push_q, pop_q, destroy_q},
    {"seqstack", init_seq, push_s, pop_s, destroy_s},
    {"liststack", init_ls, push_ls, pop_ls, destroy_ls},
    {"listqueue", init_lq, push_lq, pop_lq, destroy_lq}
};
static volatile uint64_t observable_checksum;

static void make_operations(Operation *ops, int batch) {
    size_t index = 0;
    for (int start = 0; start < PAIRS; start += batch) {
        int count = PAIRS - start < batch ? PAIRS - start : batch;
        for (int j = 0; j < count; ++j) ops[index++] = (Operation){1, start + j};
        for (int j = 0; j < count; ++j) ops[index++] = (Operation){0, 0};
    }
}
static int measure(const Adapter *a, const Operation *ops, double *ms, uint64_t *checksum) {
    Storage storage;
    if (a->init(&storage)) return -1;
    uint64_t sum = 0;
    double start = now_ms();
    for (size_t i = 0; i < (size_t)PAIRS * 2U; ++i) {
        if (ops[i].insert) {
            if (a->push(&storage, ops[i].value)) { a->destroy(&storage); return -1; }
        } else {
            int value;
            if (a->pop(&storage, &value) || value < 0) { a->destroy(&storage); return -1; }
            sum += (uint64_t)(unsigned int)value;
        }
    }
    double stop = now_ms();
    int extra;
    int is_empty = a->pop(&storage, &extra) == -1;
    a->destroy(&storage);
    observable_checksum = sum;
    if (!is_empty || sum != (uint64_t)PAIRS * (PAIRS - 1U) / 2U) return -1;
    *checksum = sum;
    *ms = stop - start;
    return 0;
}
int main(void) {
#ifdef _WIN32
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
    if (!QueryPerformanceFrequency(&frequency)) return EXIT_FAILURE;
#endif
    Operation *ops = malloc((size_t)PAIRS * 2U * sizeof(*ops));
    if (!ops) return EXIT_FAILURE;
    puts("scenario,structure,phase,round,pairs,operations,ms,checksum");
    for (int scenario = 0; scenario < 2; ++scenario) {
        const char *name = scenario ? "batch64" : "alternating";
        make_operations(ops, scenario ? LIMIT : 1);
        for (int round = 0; round <= ROUNDS; ++round) {
            for (size_t k = 0; k < sizeof(adapters)/sizeof(*adapters); ++k) {
                size_t i = (k + (size_t)round) % (sizeof(adapters)/sizeof(*adapters));
                double ms;
                uint64_t checksum;
                if (measure(&adapters[i], ops, &ms, &checksum)) {
                    fprintf(stderr, "benchmark failed: %s %s\n", name, adapters[i].name);
                    free(ops);
                    return EXIT_FAILURE;
                }
                printf("%s,%s,%s,%d,%d,%d,%.6f,%llu\n", name, adapters[i].name,
                    round ? "measured" : "warmup", round, PAIRS, PAIRS * 2, ms,
                    (unsigned long long)checksum);
            }
        }
    }
    free(ops);
    return EXIT_SUCCESS;
}
