#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <kernel_utils.h>
#include <serializacion.h>
#include <shared_utils.h>

// funciones
/**
 * @brief  Arranca el planificador de largo plazo, crea las colas de new y la de exit
 *
 */
void iniciar_planificador_largo_plazo(void);

void iniciar_planificador_corto_plazo(void);

void planificar_largo();

void planificador_corto_FIFO();

void planificador_corto_RR();

void planificador_corto_FEEDBACK();

void esperar_quantum();

/**
 * @brief Devuelve el primer elemento de la cola, usando su semaforo
 *
 * @param cola
 * @param semaforo puntero al semaforo
 * @return void* el primer elemento, se debe castear a lo que sea
 */
void* tomar_primero(t_queue* cola, pthread_mutex_t* semaforo);

void algoritmo_FIFO(t_queue* cola, pthread_mutex_t* semaforo);

void algoritmo_RR(t_queue* cola, pthread_mutex_t* semaforo);

void escuchar_mensaje_cpu();

void recibir_pcb_cpu_RR();

void recibir_pcb_cpu_FIFO();

void manejar_bloqueo(t_solicitud_io* solicitud);
void manejar_page_fault();
void esperar_carga_pagina(t_pf_request* pf);


void terminar_proceso(op_code caso);

void io_teclado(t_solicitud_io* solicitud);

void io_pantalla(t_solicitud_io* solicitud);

void io_otros_dispositivos(t_cola_bloqueo* cola_bloqueo);
// similar a destruir pero mantiene el pcb
#endif