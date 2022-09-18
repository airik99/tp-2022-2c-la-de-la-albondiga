#ifndef MEMORIA_H
#define MEMORIA_H

#include "shared_utils.h"
#include "tests.h"

typedef struct config_memoria {
	int puerto;
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

#endif

