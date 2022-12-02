#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
    int largo_nombre;
    char* nombre;
    int cant_params;
    t_list* params;
} t_instruccion;

typedef struct {
    t_list* espacios_memoria;
    t_list* instrucciones;
} t_proceso;

typedef enum estado {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    EXIT,
} t_estado;

typedef struct registro_cpu{
    u_int32_t AX;
    u_int32_t BX;
    u_int32_t CX;
    u_int32_t DX;
} registro_cpu;

typedef struct t_segmento{
    u_int32_t nro_segmento;
    u_int32_t tamanio_segmento;
    u_int32_t indice_tabla_paginas;
} t_segmento;

typedef struct t_pcb
{
    u_int32_t pid;
    u_int32_t socket_consola;
    t_list* instrucciones;
    u_int32_t program_counter;
    int registro[4];
    t_estado estado_actual;
    t_estado estado_anterior;
    t_list* tabla_segmentos;
} t_pcb;

typedef struct t_generaron_page_fault {
    u_int32_t num_segmento;
    u_int32_t num_pagina;
} t_generaron_page_fault;


typedef struct t_solicitud_io{
    char* dispositivo;
    t_pcb* pcb;
    char* parametro;
} t_solicitud_io;

// TODO Puede ser que se cambie

void print_valores(int valor);
void destructor_instrucciones(t_list*);
void destructor_instruccion(t_instruccion*);
void print_instruccion(t_instruccion*);

/**
 * @brief Agrega un elemento a la cola, usando el semaforo correspondiente
 *
 * @param q cola donde pushear el dato
 * @param dato dato a agregar
 * @param mx mutex de la cola
 *
 */
void pushear_semaforizado(t_queue* q, void* dato, pthread_mutex_t mx);
/**
 * @brief Libera todos los campos de un pcb
 *
 * @param pcb pcb a eliminar
 */
void eliminar_pcb(t_pcb* pcb);
int indice_registro(char* registro);

#endif
