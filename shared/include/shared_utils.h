#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
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

typedef struct 
{
	u_int32_t pid;
	t_list* instrucciones; 
	u_int32_t program_counter;
	//u_int32_t* registros_cpu;
	t_estado estado_actual;
	t_estado estado_anterior;
	t_list* parametros;
	// tabla_de_segmentos
} t_pcb;

typedef struct
{
	registro_cpu registro;
	u_int32_t valor;
} t_parametros;

//TODO Puede ser que se cambie
typedef enum registro_cpu{
	AX,
	BX,
	CX,
	DX
} registro_cpu;

void print_valores(int valor);
void destructor_instrucciones(t_list *);
void destructor_instruccion(t_instruccion*);
void print_instruccion(t_instruccion* );
void actualizar_estado(t_pcb*,t_estado);
#endif



