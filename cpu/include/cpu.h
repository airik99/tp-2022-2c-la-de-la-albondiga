#ifndef CPU_H
#define CPU_H

#include "cpu_utils.h"

void error_conexion(int); //evalua si la conexion no fue exitosa y tira un error

void recibir_codigo_operacion_por_dispatch(int); //si el codigo de operacion es un pcb lo guarda en la variable global pcb_recibido

void esperar_kernel_interrupt();

void esperar_kernel_dispatch();

void* conexion_inicial_memoria(); //se inicia un hilo de conexion con memoria y se envia/recibe un handshake

void pedir_handshake(int); //pide el handshake a memoria

t_handshake* recibir_handshake(int); //recibe el handshake de memoria

void* conexion_interrupciones(char*, char*); //si el codigo de operacion es una interrupcion, se debe parar el proceso (todavia no esta hecho)

void casos_de_error(int); //caso de error si el codigo de operacion no es valido
#endif