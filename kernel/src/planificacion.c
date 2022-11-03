#include "planificacion.h"

sem_t sem_cola_new;
sem_t sem_cola_ready_fifo;
sem_t sem_cola_ready_round_robin;
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

void iniciar_planificador_corto_plazo(void) {
    colaReadyRoundRobin = list_create();
    colaReadyFifo = list_create();
    colaExec = list_create();
    colaBlock = list_create();
	sem_init(&sem_cola_ready_fifo, 0, 1);
    sem_init(&sem_cola_ready_round_robin, 0, 1);
	sem_init(&sem_cola_exec, 0, 1);
	sem_init(&sem_cola_block, 0, 1);
}

void planificar_largo(t_pcb* pcb_a_planificar) {
    if(pcb_a_planificar->estado_actual==EXIT){
        free(pcb_a_planificar);
        //dar aviso al modulo Memoria para que este libere sus estructuras. Una vez liberadas, se dara aviso a la Consola de la finalizacion del proceso.
        return; 
    }
    if(pcb_a_planificar->estado_actual==PAGE_FAULT){
        //obtener la pagina "la misma sera obtenida desde el mensaje recibido de la CPU." se puede meter en el pcb como "pag_necesaria"
        log_info(logger, "page fault pid: %d - segmento:<segmento> - pagina: <pagina>  \n", pcb_a_planificar->pid);
        //page_fault(pcb_a_planificar,pagina)
        return;
    }
    sem_wait(&sem_cola_new);
    list_add(colaNew, pcb_a_planificar);
    sem_post(&sem_cola_new);
    log_info(logger, "Se agrego el pcb de pid: %d a la cola de new \n", pcb_a_planificar->pid);
    
    //sem_wait(sem_grado_multiprogramacion)
    if ((list_size(colaReadyRoundRobin) + list_size(colaReadyFifo)) < config_valores.grado_max_multiprogramacion) {
        t_pcb* primer_pcb = list_get(colaNew, 0);

        sem_wait(&sem_cola_new);
        list_remove(colaNew, 0);
        sem_post(&sem_cola_new);
        
        // TODO Enviar mensaje a memoria para que inicialice sus estructuras necesarias y obtenga el indice/identificador de la tabla de paginas 
        //de cada segmento que deberan estar almacenados en la tabla de segmentos del PCB
        log_info(logger, "Pcb de pid: %d enviado a planificar corto \n", primer_pcb->pid);
        planificador_corto(primer_pcb);
    }else{
        log_info(logger, "No se pudo enviar a planificar corto el pcb de pid: %d porque la cola de ready esta llena \n", pcb_a_planificar->pid);
    }
}

void planificador_corto(t_pcb* pcb_nuevo) {
    log_info(logger, "Se agrego el pcb de pid: %d a la cola de ready \n", pcb_nuevo->pid);
    t_pcb* pcb_a_ejecutar = malloc(sizeof(t_pcb));
    switch (config_valores.algoritmo_planificacion) {
        case FIFO:
            sem_wait(&sem_cola_ready_fifo);
            list_add(colaReadyFifo, pcb_nuevo);
            sem_post(&sem_cola_ready_fifo);
            // Obtengo el primer pcb de la cola de new, lo mando al planificador corto y lo elimino de la cola new
            pcb_a_ejecutar =algoritmo_fifo();
            break;
        case RR:
            sem_wait(&sem_cola_ready_round_robin);
            list_add(colaReadyRoundRobin, pcb_nuevo);
            sem_post(&sem_cola_ready_round_robin);
            // Obtengo el primer pcb de la cola de new, lo mando al planificador corto y lo elimino de la cola new
            pcb_a_ejecutar =algoritmo_rr();
            break;
        case FEEDBACK:
            // Obtengo el primer pcb de la cola de new, lo mando al planificador corto y lo elimino de la cola new
            sem_wait(&sem_cola_ready_round_robin);
            list_add(colaReadyRoundRobin, pcb_nuevo);
            sem_post(&sem_cola_ready_round_robin);
            pcb_a_ejecutar =algoritmo_feedback();
            break;
        default:
            break;
    }
    actualizar_estado(pcb_a_ejecutar,EXEC);
	//sem_post(sem_grado_multiprogramacion)
    // TODO mandar el pcb a ejecutar
}

t_pcb* algoritmo_feedback(){
    t_pcb* primer_pcb_ready = list_get(colaReadyRoundRobin, 0);
    while(primer_pcb_ready->estado_anterior != EXEC && list_size(colaReadyRoundRobin) > 0 ){

        sem_wait(&sem_cola_ready_round_robin);   
        list_remove(colaReadyRoundRobin, 0);
        sem_post(&sem_cola_ready_round_robin);

        sem_wait(&sem_cola_ready_fifo);
        list_add(colaReadyFifo, primer_pcb_ready);
        sem_post(&sem_cola_ready_fifo);
        log_ingreso_cola_ready(primer_pcb_ready->pid, "FIFO", primer_pcb_ready);

        primer_pcb_ready = list_get(colaReadyRoundRobin, 0);
    }
    if(!list_is_empty(colaReadyRoundRobin)){
       primer_pcb_ready = algoritmo_rr();
    } else{
       primer_pcb_ready = algoritmo_fifo();
    }
    return primer_pcb_ready;
}

t_pcb* algoritmo_fifo() {
    t_pcb* primer_pcb = list_get(colaReadyFifo, 0);
    sem_wait(&sem_cola_ready_fifo);
    list_remove(colaReadyFifo, 0);
    sem_post(&sem_cola_ready_fifo);
}


void esperar_quantum(t_pcb* pcb_a_ejecutar) {
    usleep(config_valores.quantum_rr);
    log_info(logger, "Se agrego el pcb de pid: %d a la cola de ready \n", pcb_a_ejecutar->pid);
    //TODO ENVIAR A CPU POR EL PUERTO INTERRUPT
    //t_pcb* primer_pcb = list_get(cola, 0);
    t_pcb* primer_pcb = list_get(colaReadyRoundRobin, 0);
    //sem_wait(sem_cola_ready)
    sem_wait(&sem_cola_ready_round_robin); 
    list_remove(colaReadyRoundRobin, 0);
    list_add(colaReadyRoundRobin, primer_pcb);
    sem_post(&sem_cola_ready_round_robin);
    log_ingreso_cola_ready(primer_pcb->pid, "Round Robin", primer_pcb);
    //sem_post(sem_cola_ready)
    pcb_a_ejecutar = primer_pcb;
}

t_pcb* algoritmo_rr(){
    t_pcb* pcb_a_ejecutar;
    pthread_create(hilo_proceso, NULL, esperar_quantum, &pcb_a_ejecutar);
    return pcb_a_ejecutar;
}


void atender_page_fault(t_pcb* pcb_pf,int nro_pagina){
    sem_init(&sem_page_fault, 0, 1);
    actualizar_estado(pcb_pf,BLOCKED);
    //sem_wait(sem_page_fault);
    solicitar_pagina_a_memoria(nro_pagina);
    // Al recibir la respuesta del modulo memoria, desbloquear el proceso y colocarlo en la cola de ready correspondiente.
    actualizar_estado(pcb_pf,READY);
    planificador_corto(pcb_pf);
}

t_pcb* page_fault(t_pcb* pcb_pf,int nro_pagina){
    pthread_create(hilo_page_fault,NULL,atender_page_fault,(pcb_pf,nro_pagina));
}

void solicitar_pagina_a_memoria(int nro_pagina){

    //Solicitar al modulo memoria que se cargue en memoria principal la pagina correspondiente, la misma sera obtenida desde el mensaje recibido de la CPU
    //sem_post(sem_page_fault);
}

void escuchar_mensaje_cpu() {
    // TODO Cuando reciba un mensaje del cpu para finalizar el proceso, llama a finalizar_pcb()
    // finalizar_pcb(pcb_recibido);
}

void finalizar_pcb(t_pcb* pcb) {
    actualizar_estado(pcb,EXIT);
	//sem_wait(sem_cola_exit)
    list_add(colaExit, pcb);
	//sem_post(sem_cola_exit)
    // TODO Enviar mensaje a memoria para que libere sus estructuras
    chequear_lista_pcbs(colaExit);
}

void log_ingreso_cola_ready(char* algoritmo, t_list* cola_ready, t_pcb* pcb) {
    log_info(logger, "Se agrego el pcb de pid: %d a la cola de ready con algoritmo %s \n", pcb->pid, algoritmo);
    //Crear un array con los pid de los pcb en la cola de ready
    int* pid_array = malloc(sizeof(int) * list_size(cola_ready));
    int i = 0;
    for (i = 0; i < list_size(cola_ready); i++) {
        t_pcb* pcb = list_get(cola_ready, i);
        pid_array[i] = pcb->pid;
    }
    log_info(logger, "Cola de ready con algoritmo %s: %s \n", algoritmo, string_itoa(pid_array));
    log_info(logger, "Cola ready %s: tiene %d elementos \n", algoritmo, list_size(cola_ready));
}

void chequear_lista_pcbs(t_list* lista) {  // Funcion para listar la lista de los pcb
    for (int i = 0; i < list_size(lista); i++) {
        t_pcb* pcb = list_get(lista, i);
        log_info(logger, "PCB ID: %d\n", pcb->pid);
    }
}

