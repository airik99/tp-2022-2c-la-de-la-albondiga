#ifndef MEMORIA_UTILS_H
#define MEMORIA_UTILS_H

#include "conexion.h"
#include "serializacion.h"
#include "shared_utils.h"
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
} config_memoria;

typedef struct entrada_tablas_paginas {
    int pid;
    int segmento;
    t_list* tabla_de_paginas;
} entrada_tablas_paginas;

typedef struct t_pagina {
    int marco;
    int presencia;
    int uso;
    int modificado;
    int posicion_swap;
} t_pagina;

typedef struct t_marco {
    t_pagina* pagina;
    int pid;
    int segmento;
    int numero_pagina;
} t_marco;

typedef struct proceso_en_memoria {
    int pid;
    t_list* lista_marcos_asignados;
    int indice_ptro_remplazo;
} proceso_en_memoria;

void cargar_configuracion(char* path);
bool hay_marcos_disponibles(proceso_en_memoria*);
int cantidad_marcos_asignados(proceso_en_memoria*);
int primero_libre(t_bitarray* bitarray, int cantidad_elementos);
bool coincide_pid(proceso_en_memoria* a, proceso_en_memoria* b);
bool menor_pid(proceso_en_memoria* a, proceso_en_memoria* b);
proceso_en_memoria* obtener_proceso_por_pid(int pid);
void manejador_seniales(int senial); 
void borrar_todo();
void eliminar_proceso_en_memoria(proceso_en_memoria* p);
void eliminar_entrada_tabla_pagina(entrada_tablas_paginas* tp);
void eliminar_tabla_pagina(t_pagina* p);

/**
 * @brief Puntero a funcion de reemplazo, se asigna segun el valor de config;
 *
 */

extern int (*algoritmo_reemplazo)(proceso_en_memoria*);

extern config_memoria config_valores;
extern t_config* config;
extern t_log* logger;
extern t_list *tablas_paginas, *procesos_cargados, *lista_marcos;
extern int cantidad_marcos_libres;
extern void *espacio_memoria, *marcos_libres, *swap_libre;
extern FILE* fp;
extern t_bitarray* bit_array_swap;
extern t_bitarray* bit_array_marcos_libres;
extern pthread_t manejar_conexion_cpu, manejar_conexion_kernel;
extern int socket_cpu, socket_kernel, socket_servidor;
extern pthread_mutex_t mx_conexion; 

#endif
