#include "planificacion.h"
	
t_log* logger;

void iniciar_planificador_largo_plazo(void) {

	contador_id = 0;
	colaNew = list_create();
	colaExit = list_create();
}

void planificar_largo(t_paquete_deserializado paquete) {

	iniciar_planificador_largo_plazo();
	pcb* pcb_a_planificar=crear_nuevo_pcb(paquete);
	list_add(colaNew, pcb_a_planificar);
	log_info(logger, "Se agrego el pcb a la cola de new");
	//log_info(logger, "El id del pcb es: %d", pcb_a_planificar->id);
}

pcb *crear_nuevo_pcb(t_paquete_deserializado paquete_consola) {

	pcb *nuevo_pcb = malloc(sizeof(pcb));
	nuevo_pcb->id = contador_id;
	contador_id++;
	//no es una lista el stream
	nuevo_pcb->instrucciones = list_duplicate(paquete_consola.instrucciones);
	nuevo_pcb->program_counter = 0;
	nuevo_pcb->estado_actual = NEW;
	nuevo_pcb->estado_anterior = NULL;
	return nuevo_pcb;
}

void pasar_a_exit(pcb* pcb) {
	pcb->estado_actual = EXIT;
	list_add(colaExit, pcb);
	chequear_lista_pcbs(colaExit);
}

void chequear_lista_pcbs(t_list* lista) {
    for (int i = 0 ; i < list_size(lista) ; i++){
        pcb* proceso = list_get(lista, i);
        log_info(logger,"PCB ID: %d\n", proceso->id);
    }
}



