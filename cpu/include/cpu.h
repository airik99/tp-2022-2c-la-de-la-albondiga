#ifndef CPU_H
#define CPU_H

#include "shared_utils.h"
#include "tests.h"
typedef struct config_cpu {
	int entradas_tlb;
	char* reemplazo_tlb;
	int retardo_instruccion;
	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha_dispatch;
	int puerto_escucha_interrupt;
} config_cpu;

void cargar_configuracion();

#endif