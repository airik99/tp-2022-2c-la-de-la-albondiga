
#include <shared_utils.h>
#include <conexion.h>
#include <serialiacion.h>


void iniciar_planificador_largo_plazo(void) {

	generador_de_id = 0;
	colaNew = list_create();
	colaExit = list_create();
}

pcb *crear_nuevo_pcb(t_paquete paquete_consola) {

	pcb *nuevo_pcb = malloc(sizeof(pcb));
	nuevo_pcb->id = contador_id;
	contador_id++;
	nuevo_pcb->tamanio_proceso =paquete_consola.buffer.size;
	nuevo_pcb->instrucciones = list_duplicate(paquete_consola->buffer.stream);
	nuevo_pcb->program_counter = 0;

	return nuevo_pcb;
}





