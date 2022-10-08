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

typedef enum registro_cpu{
	AX,
	BX,
	CX,
	DX
} registro_cpu;

void cargar_configuracion(); //carga todo lo del archivo de configuracion del cpu
void error_conexion(int); //evalua si la conexion no fue exitosa y tira un error

void recibir_codigo_operacion_por_dispatch(int); //si el codigo de operacion es un pcb lo guarda en la variable global pcb_recibido

void evaluar_cod_op(int); //evalua el codigo de operacion recibido. Puede ser una interrupcion, un pcb o un handshake

void* conexion_inicial_memoria(); //se inicia un hilo de conexion con memoria y se envia/recibe un handshake

void pedir_handshake(int); //pide el handshake a memoria

t_handshake* recibir_handshake(int); //recibe el handshake de memoria

void* conexion_interrupciones(char*, char*); //si el codigo de operacion es una interrupcion, se debe parar el proceso (todavia no esta hecho)

void casos_de_error(int); //caso de error si el codigo de operacion no es valido

void liberar_todo(); //liber conexiones, estructuras, logger, etc...

#endif