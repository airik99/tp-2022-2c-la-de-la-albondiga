#ifndef CPU_H
#define CPU_H

#include "shared_utils.h"
#include "tests.h"
#include "conexion.h"
#include "serializacion.h"

typedef struct config_cpu {
	int entradas_tlb;
	char* reemplazo_tlb;
	int retardo_instruccion;
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;
} config_cpu;


void cargar_configuracion(); //carga todo lo del archivo de configuracion del cpu

void error_conexion(int); //evalua si la conexion no fue exitosa y tira un error

void recibir_codigo_operacion_por_dispatch(int); //si el codigo de operacion es un pcb lo guarda en la variable global pcb_recibido

void esperar_kernel_interrupt(int);

void esperar_kernel_dispatch(int);

void evaluar_cod_op(int); //evalua el codigo de operacion recibido. Puede ser una interrupcion, un pcb o un handshake

void ciclo_de_instruccion(t_pcb* pcb); //ciclo de instruccion completo, hace el fetch, decode, execute

int checkInterrupt(); //chequea que el proceso no esté bloqueado para seguir o no, devuelve 1 si para y devuelve 0 si sigue

void decode(t_instruccion*, t_pcb*); //se fija que instruccion tiene el pcb y ejecuta la funcion asociada a esa instruccion


void ejecutar_SET(char*, uint32_t); //Asigna al registro el valor pasado como parámetro.

void ejecutar_ADD(char*, char*); //Suma ambos registros y deja el resultado en el Registro Destino.

void ejecutar_IO(char*, char*,t_pcb*); //representa una syscall de I/O bloqueante. Devuelve el Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea utilizar el proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).

void ejecutar_EXIT(t_pcb*); //Representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización

void* conexion_inicial_memoria(); //se inicia un hilo de conexion con memoria y se envia/recibe un handshake

void pedir_handshake(int); //pide el handshake a memoria

t_handshake* recibir_handshake(int); //recibe el handshake de memoria

void* conexion_interrupciones(char*, char*); //si el codigo de operacion es una interrupcion, se debe parar el proceso (todavia no esta hecho)

void casos_de_error(int); //caso de error si el codigo de operacion no es valido

void liberar_todo(); //liber conexiones, estructuras, logger, etc...

void copiar_valores_registros(int* origen, int* destino);
#endif