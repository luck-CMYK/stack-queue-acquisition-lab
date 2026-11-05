/* Per-translation-unit instrumentation: the real allocator owns every block.
 * This audits project malloc/free calls; it is not a system-wide leak tool.
 * Never linked into the benchmark, so timings contain no audit overhead.
 */
#include <stdio.h>
#include <stdlib.h>

#define AUDIT_SLOTS 16384U
static void *blocks[AUDIT_SLOTS];
static size_t live, peak, allocations;

void *audit_malloc(size_t size);
void audit_free(void *pointer);

void *audit_malloc(size_t size) {
    void *pointer = malloc(size);
    if (pointer) {
        size_t slot;
        for (slot = 0; slot < AUDIT_SLOTS && blocks[slot]; ++slot) {}
        if (slot == AUDIT_SLOTS) {
            fputs("allocation audit capacity exceeded\n", stderr);
            free(pointer);
            exit(EXIT_FAILURE);
        }
        blocks[slot] = pointer;
        ++live; ++allocations;
        if (live > peak) peak = live;
    }
    return pointer;
}
void audit_free(void *pointer) {
    if (pointer) {
        size_t slot;
        for (slot = 0; slot < AUDIT_SLOTS && blocks[slot] != pointer; ++slot) {}
        if (slot == AUDIT_SLOTS) {
            fputs("free of untracked/already freed pointer\n", stderr);
            exit(EXIT_FAILURE);
        }
        blocks[slot] = NULL;
        --live;
    }
    free(pointer);
}
static void audit_finish(void) __attribute__((destructor));
static void audit_finish(void) {
    printf("MEMORY AUDIT: allocations=%zu peak_blocks=%zu outstanding=%zu\n", allocations, peak, live);
    if (live) { fflush(NULL); _Exit(EXIT_FAILURE); }
}
