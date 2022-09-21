#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/txt.h>
#include <commons/config.h>

typedef struct
{
	char *nombre;
	t_list *params;
} t_instruccion;

void print_instruccion(t_instruccion *);
void destructor_instrucciones(t_list *);
void destructor_instruccion(t_instruccion *);

#endif