#ifndef KERNEL_H
#define KERNEL_H

#include <shared_utils.h>
#include <conexion.h>
#include <serializacion.h>
#include "tests.h"

//estructuras
typedef struct config_kernel
{
	char *ip_memoria;
	char *puerto_memoria;
	char *ip_cpu;
	char *puerto_cpu_dispatch;
	char *puerto_cpu_interrupt;
	char *puerto_escucha;
	char *algoritmo_planificacion;
	int grado_max_multiprogramacion;
	int quantum_rr;
	char **dispositivos_io;
	char **tiempos_io;
} config_kernel;

//funciones
void cargar_configuracion();

/**
 * @DESC: recibe el socket de la consola, pide el packete y lo deserializa
 *
 * @PARAMS:
 *     +socket_cliente: socket del cliente.
 *
 * @RETURN: Un paquete deserializado.
 */
t_paquete_deserializado *deserializar_consola(int);

/**
 * @DESC: Crea un paquete con el codigo de operacion indicado
 *
 * @PARAMS:
 *     +stream_datos: donde se aloja la informacion.
 * 	   +size_stream_datos: El tamaño del stream de datos.
 *
 * @RETURN: una lista con las instrucciones.
 */
t_list *deserializar_instrucciones(t_list *, uint32_t );

#endif