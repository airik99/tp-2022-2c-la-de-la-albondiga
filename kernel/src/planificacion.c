#include "planificacion.h"

pthread_mutex_t mx_cola_new;
pthread_mutex_t mx_cola_ready_prioritaria;
pthread_mutex_t mx_cola_ready_segunda;
pthread_mutex_t mx_cola_exec;
pthread_mutex_t mx_cola_block_disco;
pthread_mutex_t mx_cola_block_impresora;
pthread_mutex_t mx_cola_exit;
sem_t sem_procesos_new;
sem_t sem_procesos_ready;
sem_t sem_grado_multiprogramacion;
sem_t sem_page_fault;
sem_t sem_block_disco;
sem_t sem_block_impresora;
pthread_t t_page_fault;
pthread_t t_quantum;
pthread_t t_impresora;
pthread_t t_disco;

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
    cola_block_disco = queue_create();
    cola_block_impresora = queue_create();    // esta hardcodeado, creo que se resuleve usando vectores y eso pero ahora no puede hacerlo
    cola_ready_prioritaria = queue_create();  // igual

    pthread_create(&t_impresora, NULL, (void*)manejar_impresora, NULL);
    pthread_detach(t_impresora);

    pthread_t t_disco;
    pthread_create(&t_disco, NULL, (void*)manejar_disco, NULL);
    pthread_detach(t_disco);

    pthread_mutex_init(&mx_cola_ready_prioritaria, NULL);
    pthread_mutex_init(&mx_cola_exec, NULL);
    pthread_mutex_init(&mx_cola_block_disco, NULL);
    pthread_mutex_init(&mx_cola_block_impresora, NULL);
    sem_init(&sem_block_disco, 0, 0);
    sem_init(&sem_block_impresora, 0, 0);
    sem_init(&sem_procesos_ready, 0, 0);

    pthread_t t_corto_plazo;

    if (es_algoritmo_FEEDBACK()) {
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_FEEDBACK, NULL);
        cola_ready_segundo_nivel = queue_create();
        pthread_mutex_init(&mx_cola_ready_segunda, NULL);
    } else if (es_algoritmo_FIFO()) {
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
        t_pcb* pcb = tomar_primer_pcb(cola_new, mx_cola_new);

        sem_wait(&sem_grado_multiprogramacion);
        pthread_mutex_lock(&mx_cola_ready_prioritaria);
        queue_push(cola_ready_prioritaria, pcb);  // En RR y FIFO hay una sola cola, En Feedback los nuevos siempre entran a la de maxima prioridad
        pthread_mutex_unlock(&mx_cola_ready_prioritaria);

        actualizar_estado(pcb, READY);
        loggear_colas_ready();
        sem_post(&sem_procesos_ready);
    }
}

//----------FIFO--------------------
void planificador_corto_FIFO() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_FIFO(cola_ready_prioritaria, mx_cola_ready_prioritaria);
        recibir_pcb_cpu_FIFO();
    }
}

void algoritmo_FIFO(t_queue* queue, pthread_mutex_t semaforo) {
    t_pcb* pcb = tomar_primer_pcb(queue, semaforo);
    actualizar_estado(pcb, EXEC);
    enviar_pcb(pcb, PCB, conexion_cpu_dispatch);
    eliminar_pcb(pcb);
}

void recibir_pcb_cpu_FIFO() {
    op_code cod_op = recibir_operacion(conexion_cpu_dispatch);
    t_pcb* pcb;
    t_solicitud_io* solicitud;
    switch (cod_op) {
        case PCB_EXIT:
            pcb = recibir_pcb(conexion_cpu_dispatch);
            actualizar_estado(pcb, EXIT);
            sem_post(&sem_grado_multiprogramacion);
            op_code codigo_exit = PCB_EXIT;
            send(pcb->socket_consola, &codigo_exit, sizeof(op_code), MSG_WAITALL);
            eliminar_pcb(pcb);
            break;
        case PCB_BLOCK:
            solicitud = recibir_pcb_io(conexion_cpu_dispatch);
            manejar_bloqueo(solicitud);
            break;
        default:
            log_warning(logger, "Operacion desconocida.");
            break;
    }
}

//--------RR------------

void planificador_corto_RR() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_RR(cola_ready_prioritaria, mx_cola_ready_prioritaria);
        recibir_pcb_cpu_RR();
    }
}

void algoritmo_RR(t_queue* queue, pthread_mutex_t semaforo) {
    t_pcb* pcb = tomar_primer_pcb(queue, semaforo);
    actualizar_estado(pcb, EXEC);
    enviar_pcb(pcb, PCB, conexion_cpu_dispatch);
    pthread_create(&t_quantum, NULL, (void*)esperar_quantum, NULL);
    pthread_detach(t_quantum);
    eliminar_pcb(pcb);
}

void recibir_pcb_cpu_RR() {
    int cod_op = recibir_operacion(conexion_cpu_dispatch);
    t_pcb* pcb;
    t_solicitud_io* solicitud;
    switch (cod_op) {
        case PCB_EXIT:
            pthread_cancel(t_quantum);
            pcb = recibir_pcb(conexion_cpu_dispatch);
            actualizar_estado(pcb, EXIT);
            sem_post(&sem_grado_multiprogramacion);
            op_code codigo_exit = PCB_EXIT;
            send(pcb->socket_consola, &codigo_exit, sizeof(uint32_t), MSG_WAITALL);
            eliminar_pcb(pcb);
            break;
        case PCB_BLOCK:
            pthread_cancel(t_quantum);
            solicitud = recibir_pcb_io(conexion_cpu_dispatch);
            manejar_bloqueo(solicitud);
            break;
        case INTERRUPCION:
            pcb = recibir_pcb(conexion_cpu_dispatch);
            actualizar_estado(pcb, READY);
            sem_post(&sem_procesos_ready);
            log_info(logger, "PID: <%d> - Desalojado por fin de Quantum", pcb->pid);
            es_algoritmo_FEEDBACK() ? queue_push(cola_ready_segundo_nivel, pcb) : queue_push(cola_ready_prioritaria, pcb);
            loggear_colas_ready();
            break;
        default:
            log_warning(logger, "Operacion desconocida.");
            break;
    }
}

void esperar_quantum() {
    usleep(config_valores.quantum_rr * 1000);
    int interrupcion = 1;
    send(conexion_cpu_interrupt, &interrupcion, sizeof(uint32_t), MSG_WAITALL);
    log_info(logger, "Interrupcion por quantum enviada.");
}

//--------------------FEEDBACK--------------------------------
void planificador_corto_FEEDBACK() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        if (queue_size(cola_ready_prioritaria) > 0) {
            algoritmo_RR(cola_ready_prioritaria, mx_cola_ready_prioritaria);
            recibir_pcb_cpu_RR();
        } else {
            algoritmo_FIFO(cola_ready_segundo_nivel, mx_cola_ready_segunda);
            recibir_pcb_cpu_FIFO();
        }
    }
}

//-----------------GENERALES----------------------
t_pcb* tomar_primer_pcb(t_queue* queue, pthread_mutex_t semaforo) {
    t_pcb* pcb_a_ejecutar;
    pthread_mutex_lock(&semaforo);
    pcb_a_ejecutar = queue_pop(queue);
    pthread_mutex_unlock(&semaforo);
    return pcb_a_ejecutar;
}

void manejar_bloqueo(t_solicitud_io* solicitud) {
    actualizar_estado(solicitud->pcb, BLOCKED);
    log_info(logger, "PID: <%d> - Bloqueado por: <%s>", solicitud->pcb->pid, solicitud->dispositivo);
    if ((strcmp(solicitud->dispositivo, "PANTALLA") == 0) || (strcmp(solicitud->dispositivo, "TECLADO") == 0)) {
        pthread_t t_bloqueo;
        pthread_create(&t_bloqueo, NULL, (void*)io_pantalla_teclado, (void*)solicitud);
        pthread_detach(t_bloqueo);
    } else {  // TODO: Cambiar esta cosa fea cuando haga bien la lista de dispositivos
        if (strcmp(solicitud->dispositivo, "DISCO") == 0) {
            pthread_mutex_lock(&mx_cola_block_disco);
            queue_push(cola_block_disco, solicitud);
            pthread_mutex_unlock(&mx_cola_block_disco);
            sem_post(&sem_block_disco);
        } else {
            pthread_mutex_lock(&mx_cola_block_impresora);
            queue_push(cola_block_impresora, solicitud);
            pthread_mutex_unlock(&mx_cola_block_impresora);
            sem_post(&sem_block_impresora);
        }
    }
}
void manejar_impresora() {
    while (1) {
        sem_wait(&sem_block_impresora);
        pthread_mutex_lock(&mx_cola_block_impresora);
        t_solicitud_io* solicitud = queue_pop(cola_block_impresora);
        pthread_mutex_unlock(&mx_cola_block_impresora);
        int tiempo = atoi(solicitud->parametro) * atoi(config_valores.tiempos_io[1]) * 1000;
        usleep(tiempo);
        actualizar_estado(solicitud->pcb, READY);
        queue_push(cola_ready_prioritaria, solicitud->pcb);
        loggear_colas_ready();
        sem_post(&sem_procesos_ready);
        free(solicitud->dispositivo);
        free(solicitud->parametro);
        free(solicitud);
    }
}

void manejar_disco() {
    while (1) {
        sem_wait(&sem_block_disco);
        pthread_mutex_lock(&mx_cola_block_disco);
        t_solicitud_io* solicitud = queue_pop(cola_block_disco);
        pthread_mutex_unlock(&mx_cola_block_disco);
        int tiempo = atoi(solicitud->parametro) * atoi(config_valores.tiempos_io[0]) * 1000;
        usleep(tiempo);
        actualizar_estado(solicitud->pcb, READY);
        queue_push(cola_ready_prioritaria, solicitud->pcb);
        loggear_colas_ready();
        sem_post(&sem_procesos_ready);
        free(solicitud->dispositivo);
        free(solicitud->parametro);
        free(solicitud);
    }
}
void io_pantalla_teclado(t_solicitud_io* solicitud) {
    uint32_t respuesta;
    t_pcb* pcb = solicitud->pcb;
    if (strcmp(solicitud->dispositivo, "PANTALLA") == 0) {
        t_paquete* paquete = crear_paquete(IO_PANTALLA);
        int dato = pcb->registro[indice_registro(solicitud->parametro)];
        agregar_a_paquete(paquete, &dato, sizeof(int));
        enviar_paquete(paquete, pcb->socket_consola);
        recv(pcb->socket_consola, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    } else {
        op_code codigo = IO_TECLADO;
        send(pcb->socket_consola, &codigo, sizeof(uint32_t), MSG_WAITALL);
        recv(pcb->socket_consola, &respuesta, sizeof(uint32_t), MSG_WAITALL);
        pcb->registro[indice_registro(solicitud->parametro)] = respuesta;
    }
    actualizar_estado(solicitud->pcb, READY);
    pthread_mutex_unlock(&mx_cola_ready_prioritaria);
    queue_push(cola_ready_prioritaria, solicitud->pcb);
    pthread_mutex_unlock(&mx_cola_ready_prioritaria);
    loggear_colas_ready();
    sem_post(&sem_procesos_ready);
    free(solicitud->dispositivo);
    free(solicitud->parametro);
    free(solicitud);
}

/*void atender_page_fault(t_pcb* pcb_pf, int nro_pagina) {
    sem_init(&sem_page_fault, 0, 1);
    actualizar_estado(pcb_pf, BLOCKED);
    // sem_wait(sem_page_fault);
    solicitar_pagina_a_memoria(nro_pagina);    malloc()
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