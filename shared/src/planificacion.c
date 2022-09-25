#include <planificacion.h>


void iniciar_planificador_largo_plazo(void) {

	contador_id = 0;
	colaNew = list_create();
	colaExit = list_create();
}

pcb *crear_nuevo_pcb(t_paquete paquete_consola) {

	pcb *nuevo_pcb = malloc(sizeof(pcb));
	nuevo_pcb->id = contador_id;
	contador_id++;
	//no es una lista el stream
	nuevo_pcb->instrucciones = list_duplicate(paquete_consola.buffer->stream);
	nuevo_pcb->program_counter = 0;

	return nuevo_pcb;
}





