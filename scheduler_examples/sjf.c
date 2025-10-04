#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>


void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;      // Add to the running time of the application/task
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Task finished
            // Send msg to application
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            // Application finished and can be removed (this is FIFO after all)
            free((*cpu_task));
            (*cpu_task) = NULL;
        }
    }
    if (*cpu_task == NULL) {            // If CPU is idle
        if (*cpu_task == NULL) {  // Se a CPU está ociosa (nenhum processo a correr)
            queue_elem_t *it = rq->head;       // Ponteiro para percorrer a fila (ready queue)
            queue_elem_t *best = NULL;         // Vai guardar o elemento com o menor tempo restante
            uint32_t best_remaining = UINT32_MAX; // Inicializa com o maior valor possível (vai ser substituído pelo menor encontrado)

            // Percorrer todos os elementos da fila
            while (it) {
                // Calcula o tempo restante deste processo (tempo total - já executado)
                uint32_t remaining = (it->pcb->time_ms > it->pcb->ellapsed_time_ms)
                                     ? (it->pcb->time_ms - it->pcb->ellapsed_time_ms)
                                     : 0;

                // Se este processo tem menos tempo restante que o melhor encontrado até agora
                if (remaining < best_remaining) {
                    best_remaining = remaining;  // Atualiza o menor tempo encontrado
                    best = it;                   // Guarda este elemento como o "melhor" candidato
                }

                it = it->next; // Avança para o próximo elemento da fila
            }

            // Se encontrou algum processo "best"
            if (best) {
                // Remove esse processo da fila (mas NÃO liberta o PCB)
                queue_elem_t *removed = remove_queue_elem(rq, best);

                if (removed) {  // Se conseguiu remover da fila com sucesso
                    *cpu_task = removed->pcb;            // Coloca o processo selecionado na CPU
                    (*cpu_task)->status = TASK_RUNNING;  // Atualiza estado para "em execução"
                    (*cpu_task)->slice_start_ms = current_time_ms; // Marca quando começou a correr
                    free(removed);                       // Liberta só o invólucro da fila (queue_elem), não o PCB
                } else {
                    *cpu_task = NULL; // Se falhou a remoção, mantém CPU vazia
                }
            } else {
                *cpu_task = NULL; // Se a fila estava vazia, CPU continua vazia
            }
        }

    }
}