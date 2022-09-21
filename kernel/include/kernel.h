#ifndef KERNEL_H
#define KERNEL_H

#include <shared_utils.h>
#include <conexion.h>
#include <serializacion.h>
#include "tests.h"

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

void cargar_configuracion();

#endif