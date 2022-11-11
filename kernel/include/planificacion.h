#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <serializacion.h>
#include <shared_utils.h>
#include <semaphore.h>
#include <kernel_utils.h>

//Semaforos

extern pthread_mutex_t  mx_cola_new;
extern pthread_mutex_t  mx_cola_exec;
extern pthread_mutex_t mx_cola_block_disco;
extern pthread_mutex_t mx_cola_block_impresora;
extern pthread_mutex_t  mx_cola_exit; 
extern sem_t sem_grado_multiprogramacion;
extern sem_t sem_procesos_new;


// funciones
/**
 * @brief  Arranca el planificador de largo plazo, crea las colas de new y la de exit
 *
 */
void iniciar_planificador_largo_plazo(void);

void iniciar_planificador_corto_plazo (void);

void planificar_largo();

void planificador_corto_FIFO();

void planificador_corto_RR();

void planificador_corto_FEEDBACK();

void esperar_quantum();

/**
 * @brief Retorna el primer pcb de la cola usando los semaforos correspondientes 
 * 
 * @param cola 0 para cola de mayor prioridad, 1 para cola de menor prioridad
 * @return t_pcb* 
 */
t_pcb* tomar_primer_pcb(t_queue* cola, pthread_mutex_t semaforo);

void algoritmo_FIFO(t_queue* cola, pthread_mutex_t semaforo);

void algoritmo_RR(t_queue* cola, pthread_mutex_t semaforo);

void escuchar_mensaje_cpu();

void recibir_pcb_cpu_RR();

void recibir_pcb_cpu_FIFO();

void manejar_bloqueo(t_solicitud_io* solicitud);

void manejar_impresora();

void manejar_disco();

void io_pantalla_teclado(t_solicitud_io* solicitud);

void finalizar_pcb(t_pcb* pcb);

void chequear_lista_pcbs(t_list* lista);


#endif