#include "shared_utils.h"


void print_instruccion(t_instruccion* instruccion){
	printf("%s, params(%d):", instruccion->nombre, instruccion->cant_params);
	for(int j = 0; j < list_size(instruccion->params); j++){
		char* element = list_get(instruccion->params, j);
		printf("%s ", element);
	}
	printf("\n");
}

void destructor_instrucciones(t_list* instrucciones){
	list_destroy_and_destroy_elements(instrucciones, (void *) destructor_instruccion);
}


void destructor_instruccion(t_instruccion* instruccion){
	free(instruccion->nombre);
	list_destroy_and_destroy_elements(instruccion->params, (void *)free);
	free(instruccion);
}