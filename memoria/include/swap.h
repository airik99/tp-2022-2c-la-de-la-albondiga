#ifndef SWAP_H
#define SWAP_H

#include <memoria_utils.h>

void cargar_pagina(int, int, int, int);
void descargar_pagina(t_pagina* );
int elegir_marco(proceso_en_memoria*, int, int);
int algoritmo_clock(proceso_en_memoria*);
int algoritmo_clock_mejorado(proceso_en_memoria*);
int asginar_espacio_swap();

#endif
