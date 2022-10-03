#ifndef KERNEL_H
#define KERNEL_H

#include <shared_utils.h>
#include <conexion.h>
#include <serializacion.h>
#include "tests.h"
//#include <planificacion.h>

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


/**
 * @brief Cierra todos los hilos al enviar SIGINT (ctrl+c en la consola) para poder liberar todas las estructuras y conexiones correctamente  
 * 
 * @param senial
 */
void manejador_seniales(int senial);

#endif