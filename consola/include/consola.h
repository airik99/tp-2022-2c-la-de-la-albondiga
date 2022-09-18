#ifndef CONSOLA_H
#define CONSOLA_H

#include "shared_utils.h"
#include "tests.h"

t_instruccion* crear_instruccion(char*, t_list*);
t_list* leer_archivo(char*);
char* leer_linea(FILE*);

#endif