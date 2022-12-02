#ifndef MEMORIA_H
#define MEMORIA_H

#include <memoria_utils.h>
#include <swap.h>

pthread_mutex_t mx_conexion; 

t_list* iniciar_estructuras(t_list* segmentos);
int escuchar_clientes(int);
int crear_tabla_paginas(t_list*, int);
void finalizar_proceso(int);
int obtener_marco(int, int);
void liberar_swap_pagina(t_pagina* pag);
void liberar_marcos_pagina(int);
void escribir_en_memoria(u_int32_t, int);
u_int32_t leer_memoria(int);


#endif  // MACRO
