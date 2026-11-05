#ifndef ACQUISITION_H
#define ACQUISITION_H
#include "ds/cqueue.h"
#include <stdint.h>

/* Call from one thread/event loop. For hardware, provide a reader callback.
 * reader returns 0 with one integer sample, otherwise reports source failure.
 */
typedef int (*SampleReader)(void *context, int *value);
typedef enum { ACQ_REJECT_NEW = 0, ACQ_DROP_OLDEST = 1 } AcqPolicy;
typedef enum {
    ACQ_OK = 0, ACQ_ERROR = -1, ACQ_STOPPED = -2,
    ACQ_SOURCE_ERROR = -3, ACQ_BUFFER_FULL = -4, ACQ_EMPTY = -5
} AcqStatus;
typedef struct {
    uint64_t sampled, consumed, dropped, source_errors;
} AcqStats;
typedef struct {
    CirQueue buffer;
    SampleReader reader;
    void *context;
    AcqPolicy policy;
    AcqStats stats;
    int running;
} Acquisition;

/* effective_capacity has the SAME meaning in both queue modes.
 * init accepts a new/destroyed object; destroy before reinitializing live data.
 */
int acq_init(Acquisition *a, int effective_capacity, QueueMode mode,
             AcqPolicy policy, SampleReader reader, void *context);
int acq_start(Acquisition *a);
void acq_stop(Acquisition *a);
void acq_destroy(Acquisition *a);
AcqStatus acq_poll(Acquisition *a);
/* Reading buffered samples remains possible after stop. */
AcqStatus acq_read(Acquisition *a, int *value);
size_t acq_pending(const Acquisition *a);
int acq_get_stats(const Acquisition *a, AcqStats *stats);
#endif
