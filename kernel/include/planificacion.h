#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <serializacion.h>
#include <shared_utils.h>
#include <semaphore.h>
#include <kernel_utils.h>

//Semaforos
extern sem_t sem_cola_new;
extern sem_t sem_cola_ready;
extern sem_t sem_cola_exec;
extern sem_t sem_cola_block;
extern sem_t sem_cola_exit;
extern sem_t sem_grado_multiprogramacion;


// funciones
/**
 * @brief  Arranca el planificador de largo plazo, crea las colas de new y la de exit
 *
 */
void iniciar_planificador_largo_plazo(void);

void planificar_largo(t_pcb* pcb_a_planificar);

void iniciar_planificador_corto_plazo (void);

void planificador_corto(t_pcb* pcb_nuevo);

t_pcb* algoritmo_feedback();

t_pcb* algoritmo_fifo();

t_pcb* algoritmo_rr();

void escuchar_mensaje_cpu();

void finalizar_pcb(t_pcb* pcb);

void chequear_lista_pcbs(t_list* lista);


#endif