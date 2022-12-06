#include "shared_utils.h"

void print_instruccion(t_instruccion* instruccion) {
    printf("%s, params(%d):", instruccion->nombre, instruccion->cant_params);
    for (int j = 0; j < list_size(instruccion->params); j++) {
        char* element = list_get(instruccion->params, j);
        printf("%s ", element);
    }
    printf("\n");
}


void destructor_tabla_segmentos(t_list* segmentos) {
    list_destroy_and_destroy_elements(segmentos, free);
}


void destructor_instrucciones(t_list* instrucciones) {
    list_destroy_and_destroy_elements(instrucciones, (void*)destructor_instruccion);
}

void destructor_instruccion(t_instruccion* instruccion) {
    free(instruccion->nombre);
    list_destroy_and_destroy_elements(instruccion->params, (void*)free);
    free(instruccion);
}

void eliminar_pcb(t_pcb *pcb) {
    destructor_instrucciones(pcb->instrucciones);
    destructor_tabla_segmentos(pcb->tabla_segmentos);
    free(pcb);
}

void pushear_semaforizado(t_queue* q, void* dato, pthread_mutex_t* mx){
    pthread_mutex_lock(mx);
    queue_push(q, dato);
    pthread_mutex_unlock(mx);
}


int indice_registro(char* registro) {
    if (strcmp(registro, "AX") == 0)
        return 0;
    if (strcmp(registro, "BX") == 0)
        return 1;
    if (strcmp(registro, "CX") == 0)
        return 2;
    if (strcmp(registro, "DX") == 0)
        return 3;
    return -1;    
}
