#ifndef PLANIFICACION_H
#define PLANIFICACION_H

#include <shared_utils.h>
#include <conexion.h>
#include <serializacion.h>

//funciones
/**
 * @DESC: Crea un pcb
 *
 * @PARAMS:
 *     +consola: recibe un paquete de una consola.
 *
 * @RETURN: Un pcb.
 */
pcb *crear_nuevo_pcb(t_paquete);

/**
 * @DESC: Arranca el planificador de largo plazo, crea las colas de new y la de exit
 *
 * @RETURN: void.
 */
void iniciar_planificador_largo_plazo(void);

//variables
u_int32_t contador_id;
t_list* colaNew;
t_list *colaExit;

#endif