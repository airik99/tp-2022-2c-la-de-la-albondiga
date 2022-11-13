#ifndef CONSOLA_H
#define CONSOLA_H

#include <conexion.h>
#include <serializacion.h>
#include <shared_utils.h>

typedef struct t_config_consola {
    char* ip_kernel;
    char* puerto_kernel;
    char** segmentos;
    int tiempo_pantalla;
} t_config_consola;

void iniciar_config(char* path);
t_instruccion* crear_instruccion(char*, t_list*);
t_list* leer_archivo(char*);
char* leer_linea(FILE*);
void esperar_respuesta(int);
void destruir_estructuras();

#endif