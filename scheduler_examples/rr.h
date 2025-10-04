#ifndef RR_H
#define RR_H

#include "queue.h"
#include <stdint.h>

// Definição do quantum do Round Robin
// Com TICKS_MS = 10 (do common.h), isto dá 50 ms
#ifndef RR_QUANTUM_MS
#define RR_QUANTUM_MS (5 * TICKS_MS)
#endif

void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task);

#endif // RR_H
