#ifndef MEMORIA_H
#define MEMORIA_H

#include "shared_utils.h"
#include "conexion.h"
#include "serializacion.h"
#include "tests.h"

typedef struct config_memoria {
	char* puerto;
	int tam_memoria;
	int tam_pagina;
	int entradas_por_tabla;
	int retardo_memoria;
	char* algoritmo_reemplazo;
	int marcos_por_proceso;
	int retardo_swap;
	char* path_swap;
} config_memoria;

void cargar_configuracion();
void conectar_con_clientes(void);
int escuchar_clientes();
void iterator(char*);

#endif

