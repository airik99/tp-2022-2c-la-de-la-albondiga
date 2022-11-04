#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <serializacion.h>
#include <shared_utils.h>
#include <semaphore.h>
#include <kernel_utils.h>

//Semaforos

extern pthread_mutex_t  mx_cola_new;
extern pthread_mutex_t  mx_cola_exec;
extern pthread_mutex_t  mx_cola_block;
extern pthread_mutex_t  mx_cola_exit; 
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_procesos_new;


// funciones
/**
 * @brief  Arranca el planificador de largo plazo, crea las colas de new y la de exit
 *
 */
void iniciar_planificador_largo_plazo(void);

void planificar_largo();

void recibir_pcb_cpu();

void iniciar_planificador_corto_plazo (void);

void planificador_corto();

t_pcb* algoritmo_feedback();

t_pcb* algoritmo_fifo();

t_pcb* algoritmo_rr();

void escuchar_mensaje_cpu();

void finalizar_pcb(t_pcb* pcb);

void chequear_lista_pcbs(t_list* lista);


#endif