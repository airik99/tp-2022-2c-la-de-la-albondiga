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
	int tam_swap;
	int entradas_por_tabla;
	int retardo_memoria;
	char* algoritmo_reemplazo;
	int marcos_por_proceso;
	int retardo_swap;
	char* path_swap;
	int tam_swap;
} config_memoria;

typedef struct entrada_tabla_paginas {
	int marco;
	int presencia;
	int uso;
	int modificado;
	int posicion_swap;
} entrada_tabla_paginas;

void cargar_configuracion();
void conectar_con_clientes(void);
int escuchar_clientes(int);
void iniciar_tabla(t_list* segmentos);
void iterator(char*);

/*Agrega un segmento de cierto tamaño al swap y devuelve la posicion de inicio*/
int cargar_segmento_en_swap(int ); 

#endif

