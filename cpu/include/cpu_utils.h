#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include "shared_utils.h"
#include "tests.h"
#include "conexion.h"
#include "serializacion.h"
#include <time.h>
#include <math.h>

typedef struct config_cpu {
	int entradas_tlb;
	char* reemplazo_tlb;
	int retardo_instruccion;
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;
} config_cpu;

typedef struct {
	uint32_t pid;
	uint32_t segmento;
	uint32_t pagina;
	uint32_t marco;
	time_t instante_ultima_referencia;
} t_traduccion;

// Solamente se permiten agregar campos que faciliten la implementación de los algoritmos de reemplazo como "instante de carga" o "instante de última referencia".


extern config_cpu config_valores;
extern t_config* config;
extern t_log* logger;
extern int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt;
extern pthread_t t_conexion_memoria, hilo_dispatch, hilo_interrupt, pedidofin;
extern int ultimo_pid; //esto creo que no se usa
extern int registros[4];
extern int flag_salida, interrupcion;
extern int tam_pagina, cant_entradas_por_tabla;
extern t_list* cola_lru, *tlb;
extern t_queue* cola_fifo;
extern t_pcb* pcb_actual;
extern int num_pagina_actual, num_segmento_actual;
extern pthread_mutex_t mx_traduccion_direccion_logica;

void cargar_configuracion(char* path); //carga todo lo del archivo de configuracion del cpu

void liberar_todo(); //libera conexiones, estructuras, logger, etc...

void copiar_valores_registros(int* origen, int* destino);

void manejador_seniales(int senial);

void finalizar();
#endif