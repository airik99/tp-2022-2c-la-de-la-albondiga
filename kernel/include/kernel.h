#ifndef KERNEL_H
#define KERNEL_H

#include <shared_utils.h>
#include <conexion.h>
#include <kernel_utils.h>
#include <planificacion.h>
#include "serializacion.h"
#include "tests.h"
 
//estructuras

//funciones
void cargar_configuracion();

/**
 * @brief Funcion que se encarga de esperar la conexion de una consola
 * 
 * @param socket_servidor socket del servidor
 */
void manejar_consolas(int socket_servidor);


/**
 * @brief Recibe la lista de instrucciones y segmetnos de memoria de una consola y crea el pcb del proceso
 * 
 * @param socket_cliente socket de la consola que se conecta
 */
void escuchar_consola(int socket_cliente);


#endif