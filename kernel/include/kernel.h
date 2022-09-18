#ifndef KERNEL_H
#define KERNEL_H

#include "shared_utils.h"
#include "tests.h"

typedef struct config_kernel {
	int ip_memoria;
	int puerto_memoria;
	char* ip_cpu;
	int puerto_cpu_dispatch;
    int puerto_cpu_interrupt;
	int puerto_escucha;
	char* algoritmo_planificacion;
	int grado_max_multiprogramacion;
	int quantum_rr;
	char** dispositivos_io;
    int* tiempos_io;
} config_kernel;

void cargar_configuracion();

#endif