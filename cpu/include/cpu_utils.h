#ifndef CPU_UTILS_H
#define CPU_UTILS_H

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


extern config_cpu config_valores;
extern t_config* config;
extern t_log* logger;
extern t_handshake* configuracion_tabla;
extern int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt;
extern pthread_t conexion_memoria_i, hilo_dispatch, hilo_interrupt, pedidofin;
extern int ultimo_pid;
extern int registros[4];
extern int flag_salida, interrupcion;

void cargar_configuracion(); //carga todo lo del archivo de configuracion del cpu

void liberar_todo(); //liber conexiones, estructuras, logger, etc...

void copiar_valores_registros(int* origen, int* destino);

void manejador_seniales(int senial);

void finalizar();
#endif