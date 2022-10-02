#include "planificacion.h"
#include "kernel.c"
	
t_log* logger;

void iniciar_planificador_largo_plazo(void) {

	contador_id = 0;
	colaNew = list_create();
	colaExit = list_create();
}

void planificar_largo(t_proceso paquete) {
	pcb* pcb_a_planificar=crear_nuevo_pcb(paquete);
	list_add(colaNew, pcb_a_planificar);
	log_info(logger, "Se agrego el pcb a la cola de new");

	if(list_size(colaReady) < config_valores.grado_max_multiprogramacion){
		//Obtengo el primer pcb de la cola de new, lo mando al planificador corto y lo elimino de la cola new
		pcb* primer_pcb= list_get(colaNew, 0);
		planificador_corto(primer_pcb);
		list_remove(colaNew, 0);
		log_info(logger, "Se elimino el primer pcb de la cola de new");
		//TODO Enviar mensaje a memoria para que inicialice sus estructuras necesarias
	}
}

void iniciar_planificador_corto_plazo (void){
	colaReady = list_create();
	colaExec = list_create();
	colaBlock = list_create();
}

void planificador_corto(pcb* pcb_nuevo){
	list_add(colaReady, pcb_nuevo);
	log_info(logger, "Se agrego el pcb a la cola de ready");
	pcb* pcb_a_ejecutar = algoritmo_fifo();
	pcb_a_ejecutar->estado_actual = EXEC;
	//TODO mandar el pcb a ejecutar 
}

pcb* algoritmo_fifo(){
	pcb* primer_pcb_ready = list_get(colaReady, 0);
	list_remove(colaReady, 0);
	return primer_pcb_ready;
}

pcb* crear_nuevo_pcb(t_proceso paquete_consola) {
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

void escuchar_mensaje_cpu(){
	//TODO Cuando reciba un mensaje del cpu para finalizar el proceso, llama a finalizar_pcb()
	//finalizar_pcb(pcb_recibido);
}

void finalizar_pcb(pcb* pcb) {
	pcb->estado_actual = EXIT;
	list_add(colaExit, pcb);
	//TODO Enviar mensaje a memoria para que libere sus estructuras
	chequear_lista_pcbs(colaExit);
}

void chequear_lista_pcbs(t_list* lista) { //Funcion para listar la lista de los pcb
    for (int i = 0 ; i < list_size(lista) ; i++){
        pcb* proceso = list_get(lista, i);
        log_info(logger,"PCB ID: %d\n", proceso->id);
    }
}



