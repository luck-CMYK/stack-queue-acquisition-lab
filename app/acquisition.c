#include "acquisition.h"
#include <limits.h>

static int valid(const Acquisition *a) { return a && a->buffer.data && a->reader; }
int acq_init(Acquisition *a, int cap, QueueMode mode, AcqPolicy policy, SampleReader reader, void *context) {
    if (!a) return -1;
    *a = (Acquisition){0};
    if (!reader || cap < 1 || (mode != Q_SPARE_SLOT && mode != Q_COUNTED) ||
        (policy != ACQ_REJECT_NEW && policy != ACQ_DROP_OLDEST) ||
        (mode == Q_SPARE_SLOT && cap == INT_MAX)) return -1;
    int physical_capacity = cap + (mode == Q_SPARE_SLOT);
    if (q_init_mode(&a->buffer, physical_capacity, mode)) return -1;
    a->reader = reader;
    a->context = context;
    a->policy = policy;
    return 0;
}
int acq_start(Acquisition *a) {
    if (!valid(a)) return -1;
    a->running = 1;
    return 0;
}
void acq_stop(Acquisition *a) { if (a) a->running = 0; }
void acq_destroy(Acquisition *a) {
    if (a) { q_destroy(&a->buffer); *a = (Acquisition){0}; }
}
AcqStatus acq_poll(Acquisition *a) {
    if (!valid(a)) return ACQ_ERROR;
    if (!a->running) return ACQ_STOPPED;
    int value;
    if (a->reader(a->context, &value)) { ++a->stats.source_errors; return ACQ_SOURCE_ERROR; }
    ++a->stats.sampled;
    if (q_full(&a->buffer)) {
        ++a->stats.dropped;
        if (a->policy == ACQ_REJECT_NEW) return ACQ_BUFFER_FULL;
    }
    if (q_enqueue_overwrite(&a->buffer, value)) return ACQ_ERROR;
    return ACQ_OK;
}
AcqStatus acq_read(Acquisition *a, int *value) {
    if (!valid(a) || !value) return ACQ_ERROR;
    if (q_empty(&a->buffer)) return ACQ_EMPTY;
    if (q_dequeue(&a->buffer, value)) return ACQ_ERROR;
    ++a->stats.consumed;
    return ACQ_OK;
}
size_t acq_pending(const Acquisition *a) { return a ? q_size(&a->buffer) : 0; }
int acq_get_stats(const Acquisition *a, AcqStats *stats) {
    if (!valid(a) || !stats) return -1;
    *stats = a->stats;
    return 0;
}
