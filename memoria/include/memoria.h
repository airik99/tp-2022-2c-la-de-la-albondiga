#ifndef MEMORIA_H
#define MEMORIA_H

#include <memoria_utils.h>
#include <swap.h>


t_list* iniciar_estructuras(int pid, t_list* segmentos);
int crear_tabla_paginas(t_list*, int);
void finalizar_proceso(int, t_list*);
int obtener_marco(int, int, int);
void liberar_swap_pagina(t_pagina* pag);
void escribir_en_memoria(u_int32_t, int);
u_int32_t leer_memoria(int);


#endif  // MACRO
