#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/config.h>
#include <pthread.h>

typedef struct
{
	int largo_nombre;
	char *nombre;
	int cant_params;
	t_list *params;
} t_instruccion;

typedef struct{
	t_list* espacios_memoria;
	t_list* instrucciones;
} t_proceso;

typedef enum estado {
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT,
	PAGE_FAULT
} t_estado;

typedef struct{
	u_int32_t AX;
	u_int32_t BX;
	u_int32_t CX;
	u_int32_t DX;
} registro_cpu;

typedef struct 
{
	u_int32_t pid;
	u_int32_t socket_consola;
	t_list* instrucciones; 
	u_int32_t program_counter;
	int registro[4];
	t_estado estado_actual;
	t_estado estado_anterior;
	// tabla_de_segmentos
} t_pcb;

typedef struct{
	t_pcb* pcb;
	char* dispositivo;
	char* parametro;
} t_solicitud_io;
typedef struct
{
	registro_cpu registro;
	u_int32_t valor;
} t_parametros;

//TODO Puede ser que se cambie


void print_valores(int valor);
void destructor_instrucciones(t_list *);
void destructor_instruccion(t_instruccion*);
void print_instruccion(t_instruccion* );
/**
 * @brief Libera todos los campos de un pcb
 *
 * @param pcb pcb a eliminar
 */
void eliminar_pcb(t_pcb *pcb);
int indice_registro(char* registro);

#endif



