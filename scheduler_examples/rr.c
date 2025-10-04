#include "rr.h"
#include "queue.h"
#include "msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Round Robin (RR)
 * - Executa cada processo por uma fatia fixa de tempo (quantum).
 * - Se o processo terminar antes do fim do quantum, envia DONE e liberta-o.
 * - Se o quantum expirar e o processo ainda não terminou, ele é preemptado
 *   (retirado do CPU) e colocado no FIM da ready queue.
 * - Quando a CPU fica livre, escolhe-se o próximo processo da ready queue (FIFO).
 */
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // ============================
    // 1) Atualizar a tarefa no CPU
    // ============================
    if (*cpu_task) {
        // Avança o trabalho executado nesta tarefa em TICKS_MS (o “tick” do simulador).
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // ------------------------
        // 1.a) Verifica se terminou
        // ------------------------
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            // Constrói e envia a mensagem DONE para a aplicação cliente.
            msg_t msg = {
                .pid     = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t))
                perror("write");

            // Liberta o PCB e marca a CPU como livre.
            free(*cpu_task);
            *cpu_task = NULL;
        } else {
            // ------------------------------------------------------
            // 1.b) Ainda não terminou: verificar se o quantum expirou
            // ------------------------------------------------------
            // Tempo decorrido desde o início da fatia atual.
            uint32_t ran_ms = current_time_ms - (*cpu_task)->slice_start_ms;

            // Se já gastou o quantum configurado (RR_QUANTUM_MS)...
            if (ran_ms >= RR_QUANTUM_MS) {
                // ...preempta: coloca o processo no FIM da ready queue
                // para voltar a executar na sua vez.
                (*cpu_task)->last_update_time_ms = current_time_ms;
                enqueue_pcb(rq, *cpu_task);

                // Liberta a CPU para que outro processo possa entrar.
                *cpu_task = NULL;
            }
        }
    }

    // ============================================
    // 2) Se a CPU está livre, escolher o próximo
    //    (política FIFO sobre a ready queue)
    // ============================================
    if (*cpu_task == NULL) {
        // Retira o primeiro elemento da ready queue (dequeue FIFO).
        pcb_t *next = dequeue_pcb(rq);
        if (next) {
            // Marca o processo como em execução.
            next->status = TASK_RUNNING;

            // Reinicia a contagem do quantum: começa agora.
            next->slice_start_ms = current_time_ms;

            // (Opcional) para logs/telemetria do teu sistema.
            next->last_update_time_ms = current_time_ms;

            // Coloca o processo no CPU.
            *cpu_task = next;
        }
    }
}
