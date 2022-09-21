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

void cargar_configuracion();

#endif