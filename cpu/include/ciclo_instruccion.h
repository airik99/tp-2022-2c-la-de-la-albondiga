#ifndef CPU_H
#define CPU_H

#include "cpu_utils.h"

void ciclo_de_instruccion(t_pcb* pcb);  // ciclo de instruccion completo, hace el fetch, decode, execute

int check_interrupt();  // chequea que el proceso no esté bloqueado para seguir o no, devuelve 1 si para y devuelve 0 si sigue

void decode(t_instruccion*, t_pcb*);  // se fija que instruccion tiene el pcb y ejecuta la funcion asociada a esa instruccion

void ejecutar_SET(char*, uint32_t);  // Asigna al registro el valor pasado como parámetro.

void ejecutar_ADD(char*, char*);  // Suma ambos registros y deja el resultado en el Registro Destino.

void ejecutar_IO(char*, char*, t_pcb*);  // representa una syscall de I/O bloqueante. Devuelve el Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea utilizar el proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).

void ejecutar_EXIT(t_pcb*);  // Representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización

#endif