#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* A failing assertion returns normally; no abort/crash dump is needed. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef int (*TestFn)(void);
static inline void run_test(const char *name, TestFn fn, int *total, int *failed) {
#ifdef _WIN32
    SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX);
#endif
    ++*total;
    if (fn()) { ++*failed; printf("[FAIL] %s\n", name); }
    else printf("[PASS] %s\n", name);
}
static inline int test_summary(int total, int failed) {
    printf("RESULT: %d tests, %d passed, %d failed\n", total, total - failed, failed);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif
