#include "planificacion.h"

#include "kernel_utils.h"

sem_t sem_cola_new;
sem_t sem_cola_ready;
sem_t sem_cola_exec;
sem_t sem_cola_block;
sem_t sem_cola_exit;
sem_t sem_grado_multiprogramacion;
sem_t sem_page_fault;
pthread_t hilo_proceso;
pthread_t hilo_page_fault;

void iniciar_planificador_largo_plazo(void) {
    contador_pid = 0;
    colaNew = list_create();
    colaExit = list_create();
	sem_init(&sem_cola_new, 0, 1);
	sem_init(&sem_cola_exit, 0, 1);
	sem_init(&sem_grado_multiprogramacion, 0, config_valores.grado_max_multiprogramacion);
}

void planificar_largo(t_pcb* pcb_a_planificar) {
    if(pcb_a_planificar->estado_actual==EXIT){
        free(pcb_a_planificar);
        //dar aviso al módulo Memoria para que éste libere sus estructuras. Una vez liberadas, se dará aviso a la Consola de la finalización del proceso.
    }
    else if(pcb_a_planificar->estado_actual==PAGE_FAULT){
        //obtener la pagina "la misma será obtenida desde el mensaje recibido de la CPU." se puede meter en el pcb como "pag_necesaria"
        log_info(logger, "page fault pid: %d - segmento:<segmento> - pagina: <pagina>  \n", pcb_a_planificar->pid);
        //page_fault(pcb_a_planificar,pagina)
    }
    else{
        //sem_wait(sem_cola_new)
        list_add(colaNew, pcb_a_planificar);
        //sem_post(sem_cola_new)
        log_info(logger, "Se agrego el pcb de pid: %d a la cola de new \n", pcb_a_planificar->pid);
        
        //sem_wait(sem_grado_multiprogramacion)
        if (list_size(colaReady) < config_valores.grado_max_multiprogramacion) {
            // Obtengo el primer pcb de la cola de new, lo mando al planificador corto y lo elimino de la cola new
            t_pcb* primer_pcb =algoritmo_fifo(colaNew, sem_cola_new);
            // TODO Enviar mensaje a memoria para que inicialice sus estructuras necesarias y obtenga el índice/identificador de la tabla de páginas de cada segmento que deberán estar almacenados en la tabla de segmentos del PCB
            log_info(logger, "Pcb de pid: %d enviado a planificar corto \n", primer_pcb->pid);
            planificador_corto(primer_pcb);
        }else{
            log_info(logger, "No se pudo enviar a planificar corto el pcb de pid: %d porque la cola de ready esta llena \n", pcb_a_planificar->pid);
        }
    }
}
void iniciar_planificador_corto_plazo(void) {
    colaReady = list_create();
    colaExec = list_create();
    colaBlock = list_create();
	sem_init(&sem_cola_ready, 0, 1);
	sem_init(&sem_cola_exec, 0, 1);
	sem_init(&sem_cola_block, 0, 1);
}

void planificador_corto(t_pcb* pcb_nuevo) {
	//sem_wait(sem_cola_ready)
    list_add(colaReady, pcb_nuevo);
	//sem_post(sem_cola_ready)
    log_info(logger, "Se agrego el pcb de pid: %d a la cola de ready \n", pcb_nuevo->pid);
    t_pcb* pcb_a_ejecutar = algoritmo_fifo(colaReady, sem_cola_ready);
    pcb_a_ejecutar->estado_actual = EXEC;
	//sem_post(sem_grado_multiprogramacion)
    // TODO mandar el pcb a ejecutar
}

t_pcb* algoritmo_fifo(t_list* cola, sem_t  sem_cola) {
    //t_pcb* primer_pcb_ready= malloc(sizeof(t_pcb*));
    t_pcb* primer_pcb = list_get(cola, 0);
	//sem_wait(sem_cola)
    list_remove(cola, 0);
	//sem_post(sem_cola)
    return primer_pcb;
}

t_pcb* algoritmo_rr(t_list* cola, sem_t* sem_cola){
    pthread_create(hilo_proceso, NULL, esperar_quantum, (cola, sem_cola));
}

void * esperar_quantum(t_list* cola, sem_t sem_cola){
    usleep(config_valores.quantum_rr);
    //TODO ENVIAR A CPU POR EL PUERTO INTERRUPT
    t_pcb* primer_pcb = list_get(cola, 0);
    //sem_wait(sem_cola)
    list_remove(cola, 0);
    list_add(cola, primer_pcb);
    //sem_post(sem_cola)
}

t_pcb* page_fault(t_pcb* pcb_pf,int nro_pagina){
    ptheard_create(hilo_page_fault,NULL,atender_page_fault,(pcb_pf,nro_pagina));
}

void* atender_page_fault(t_pcb* pcb_pf,int nro_pagina){
    sem_init(&sem_page_fault, 0, 1);
    pcb_pf->estado_actual=BLOCKED;
    //sem_wait(sem_page_fault);
    solicitar_pagina_a_memoria(nro_pagina);
    // Al recibir la respuesta del módulo memoria, desbloquear el proceso y colocarlo en la cola de ready correspondiente.
    pcb_pf->estado_actual=READY;
    planificador_corto(pcb_pf);
}

void* solicitar_pagina_a_memoria(int nro_pagina){

    //Solicitar al módulo memoria que se cargue en memoria principal la página correspondiente, la misma será obtenida desde el mensaje recibido de la CPU
    //sem_post(sem_page_fault);
}

void escuchar_mensaje_cpu() {
    // TODO Cuando reciba un mensaje del cpu para finalizar el proceso, llama a finalizar_pcb()
    // finalizar_pcb(pcb_recibido);
}

void finalizar_pcb(t_pcb* pcb) {
    pcb->estado_actual = EXIT;
	//sem_wait(sem_cola_exit)
    list_add(colaExit, pcb);
	//sem_post(sem_cola_exit)
    // TODO Enviar mensaje a memoria para que libere sus estructuras
    chequear_lista_pcbs(colaExit);
}

void chequear_lista_pcbs(t_list* lista) {  // Funcion para listar la lista de los pcb
    for (int i = 0; i < list_size(lista); i++) {
        t_pcb* pcb = list_get(lista, i);
        log_info(logger, "PCB ID: %d\n", pcb->pid);
    }
}
