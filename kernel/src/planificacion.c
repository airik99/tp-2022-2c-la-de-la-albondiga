#include "planificacion.h"

pthread_mutex_t mx_cola_new;
pthread_mutex_t mx_cola_ready_prioritaria;
pthread_mutex_t mx_cola_ready_segunda;
pthread_mutex_t mx_cola_exec;
pthread_mutex_t mx_cola_block;
pthread_mutex_t mx_cola_exit;
sem_t sem_procesos_new;
sem_t sem_procesos_ready;
sem_t sem_grado_multiprogramacion;
sem_t sem_page_fault;
pthread_t t_page_fault;
pthread_t t_quantum;

void iniciar_planificador_largo_plazo(void) {
    contador_pid = 0;
    cola_new = queue_create();
    cola_exit = queue_create();

    pthread_mutex_init(&mx_cola_new, NULL);
    pthread_mutex_init(&mx_cola_exit, NULL);

    sem_init(&sem_procesos_new, 0, 0);
    sem_init(&sem_grado_multiprogramacion, 0, config_valores.grado_max_multiprogramacion);

    pthread_t t_largo_plazo;
    pthread_create(&t_largo_plazo, NULL, (void*)planificar_largo, NULL);
    pthread_detach(t_largo_plazo);
}

void iniciar_planificador_corto_plazo(void) {
    cola_exec = queue_create();
    colaBlock = queue_create();
    cola_ready_prioritaria = queue_create();

    pthread_mutex_init(&mx_cola_ready_prioritaria, NULL);
    pthread_mutex_init(&mx_cola_exec, NULL);
    pthread_mutex_init(&mx_cola_block, NULL);
    sem_init(&sem_procesos_ready, 0, 0);

    pthread_t t_corto_plazo;

    if (es_algoritmo_FEEDBACK()) {
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_FEEDBACK, NULL);
        cola_ready_segundo_nivel = queue_create();
        pthread_mutex_init(&mx_cola_ready_segunda, NULL);
    }
    else if (es_algoritmo_FIFO()) {
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_FIFO, NULL);
    } else {
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_RR, NULL);
    }
    log_info(logger, "Iniciado el Planificador de corto plazo con algoritmo %s", config_valores.algoritmo_planificacion);
    pthread_detach(t_corto_plazo);
}

void planificar_largo() {
    while (1) {
        sem_wait(&sem_procesos_new);
        pthread_mutex_lock(&mx_cola_new);
        t_pcb* pcb = queue_pop(cola_new);
        pthread_mutex_unlock(&mx_cola_new);

        sem_wait(&sem_grado_multiprogramacion);
        pthread_mutex_lock(&mx_cola_ready_prioritaria);
        queue_push(cola_ready_prioritaria, pcb);  // En RR y FIFO hay una sola cola, En Feedback los nuevos siempre entran a la de maxima prioridad
        pthread_mutex_unlock(&mx_cola_ready_prioritaria);

        actualizar_estado(pcb, READY);
        //loggear_colas_ready();
        sem_post(&sem_procesos_ready);
    }
}

void planificador_corto_FIFO() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_FIFO(0);
        recibir_pcb_cpu_FIFO();
    }
}

void planificador_corto_RR() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_RR(0);
        recibir_pcb_cpu_RR();
    }
}

void planificador_corto_FEEDBACK() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        if (queue_size(cola_ready_prioritaria) > 0) {
            algoritmo_RR(0);
            recibir_pcb_cpu_RR();
        } else {
            algoritmo_FIFO(1);
            recibir_pcb_cpu_FIFO();
        }
    }
}

void algoritmo_FIFO(int cola) {
    t_pcb* pcb = tomar_primer_pcb(cola);
    actualizar_estado(pcb, EXEC);
    enviar_pcb(pcb, conexion_cpu_dispatch);
}

void algoritmo_RR(int cola) {
    algoritmo_FIFO(cola);
    pthread_create(&t_quantum, NULL, (void*)esperar_quantum, NULL);
    pthread_detach(t_quantum);
}

void recibir_pcb_cpu_FIFO() {
    int cod_op = recibir_operacion(conexion_cpu_dispatch);
    t_pcb* pcb;
    switch (cod_op) {
        case PCB_EXIT:
            pcb = recibir_pcb(conexion_cpu_dispatch);
            sem_post(&sem_grado_multiprogramacion);
            actualizar_estado(pcb,EXIT);
            eliminar_pcb(pcb);
            break;
        // Case BLOCK: Hay que ver porque hay 3 block distintos
        default:
            log_warning(logger, "El proceso %d con operacion desconocida.", pcb->pid);
            break;
    }
}

void recibir_pcb_cpu_RR() {
    int cod_op = recibir_operacion(conexion_cpu_dispatch);
    t_pcb* pcb;
    switch (cod_op) {
        case PCB_EXIT:
            pcb = recibir_pcb(conexion_cpu_dispatch);
            sem_post(&sem_grado_multiprogramacion);
            actualizar_estado(pcb,EXIT);
            eliminar_pcb(pcb);
            // pthread_cancel(t_quantum);
            break;
        // case INTERRUPCION:
        //  Case BLOCK: Hay que ver porque hay 3 block distintos
        // pthread_cancel(t_quantum)
        default:
            log_warning(logger, "Operacion desconocida.");
            break;
    }
}

t_pcb* tomar_primer_pcb(int cola) {
    t_pcb* pcb_a_ejecutar;
    if (cola == 0) {
        pthread_mutex_lock(&mx_cola_ready_prioritaria);
        pcb_a_ejecutar = queue_pop(cola_ready_prioritaria);
        pthread_mutex_unlock(&mx_cola_ready_prioritaria);
    } else if (cola == 1) {
        pthread_mutex_lock(&mx_cola_ready_segunda);
        pcb_a_ejecutar = queue_pop(cola_ready_segundo_nivel);
        pthread_mutex_unlock(&mx_cola_ready_segunda);
    } else {
        pcb_a_ejecutar = NULL;
        log_error(logger, "Parametro de cola incorrecto");
    }
    return pcb_a_ejecutar;
}

void esperar_quantum() {
    usleep(config_valores.quantum_rr);
    // enviar interrupcion;
}

/*void atender_page_fault(t_pcb* pcb_pf, int nro_pagina) {
    sem_init(&sem_page_fault, 0, 1);
    actualizar_estado(pcb_pf, BLOCKED);
    // sem_wait(sem_page_fault);
    solicitar_pagina_a_memoria(nro_pagina);
    // Al recibir la respuesta del modulo memoria, desbloquear el proceso y colocarlo en la cola de ready correspondiente.
    actualizar_estado(pcb_pf, READY);
    planificador_corto(pcb_pf);
}

t_pcb* page_fault(t_pcb* pcb_pf, int nro_pagina) {
    pthread_create(t_page_fault, NULL, atender_page_fault, (pcb_pf, nro_pagina));
}

void solicitar_pagina_a_memoria(int nro_pagina) {
    // Solicitar al modulo memoria que se cargue en memoria principal la pagina correspondiente, la misma sera obtenida desde el mensaje recibido de la CPU
    // sem_post(sem_page_fault);
}

void escuchar_mensaje_cpu() {
    // TODO Cuando reciba un mensaje del cpu para finalizar el proceso, llama a finalizar_pcb()
    // finalizar_pcb(pcb_recibido);
}

void finalizar_pcb(t_pcb* pcb) {
    actualizar_estado(pcb, EXIT);
    // sem_wait(mx_cola_exit)
    queue_push(cola_exit, pcb);
    // sem_post(mx_cola_exit)
    // TODO Enviar mensaje a memoria para que libere sus estructuras
    chequear_lista_pcbs(cola_exit);
}

void chequear_lista_pcbs(t_list* lista) {  // Funcion para listar la lista de los pcb
    for (int i = 0; i < list_size(lista); i++) {
        t_pcb* pcb = list_get(lista, i);
        log_info(logger, "PCB ID: %d\n", pcb->pid);
    }
}*/
