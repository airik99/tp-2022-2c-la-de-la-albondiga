#include "planificacion.h"

void iniciar_planificador_largo_plazo(void) {
    contador_pid = 0;
    cola_new = queue_create();

    pthread_mutex_init(&mx_cola_new, NULL);

    sem_init(&sem_procesos_new, 0, 0);
    sem_init(&sem_grado_multiprogramacion, 0, config_valores.grado_max_multiprogramacion);
    pthread_create(&t_largo_plazo, NULL, (void*)planificar_largo, NULL);
    pthread_detach(t_largo_plazo);
}

void iniciar_planificador_corto_plazo(void) {
    cola_ready_prioritaria = queue_create();
    lista_colas_bloqueo = list_create();

    for (int i = 0; config_valores.dispositivos_io[i] != NULL; i++) {
        t_cola_bloqueo* cola_bloqueo = malloc(sizeof(t_cola_bloqueo));
        cola_bloqueo->dispositivo = config_valores.dispositivos_io[i];
        cola_bloqueo->tiempo_dispositivo = atoi(config_valores.tiempos_io[i]);
        sem_init(&(cola_bloqueo->procesos_bloqueado), 0, 0);
        pthread_mutex_init(&(cola_bloqueo->mx_cola_bloqueados), NULL);
        cola_bloqueo->cola_bloqueados = queue_create();
        pthread_create(&(cola_bloqueo->t_bloqueo), NULL, (void*)io_otros_dispositivos, cola_bloqueo);
        pthread_detach(cola_bloqueo->t_bloqueo);
        list_add(lista_colas_bloqueo, cola_bloqueo);
    }

    pthread_mutex_init(&mx_cola_ready_prioritaria, NULL);
    sem_init(&sem_procesos_ready, 0, 0);

    if (es_algoritmo_FEEDBACK()) {
        pthread_mutex_init(&mx_cola_ready_segunda, NULL);
        cola_ready_segundo_nivel = queue_create();
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_FEEDBACK, NULL);
    } else if (es_algoritmo_FIFO())
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_FIFO, NULL);
    else
        pthread_create(&t_corto_plazo, NULL, (void*)planificador_corto_RR, NULL);

    pthread_detach(t_corto_plazo);
}

void planificar_largo() {
    while (1) {
        sem_wait(&sem_procesos_new);
        sem_wait(&sem_grado_multiprogramacion);
        t_pcb* pcb = tomar_primero(cola_new, &mx_cola_new);
        pthread_mutex_lock(&mx_memoria);
        enviar_pid_tamanio_segmentos(conexion_memoria, pcb);
        recibir_operacion(conexion_memoria);
        t_list* lista_ids = recibir_lista(conexion_memoria);
        pthread_mutex_unlock(&mx_memoria);
        for (int i = 0; i < list_size(lista_ids); i++) {
            int valor = list_get(lista_ids, i);
            t_segmento* segmento = list_get(pcb->tabla_segmentos, i);
            segmento->indice_tabla_paginas = valor;
        }
        list_destroy(lista_ids);
        pushear_semaforizado(cola_ready_prioritaria, pcb, &mx_cola_ready_prioritaria);
        actualizar_estado(pcb, READY);
        loggear_colas_ready();
        sem_post(&sem_procesos_ready);
    }
}

//----------FIFO--------------------
void planificador_corto_FIFO() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_FIFO(cola_ready_prioritaria, &mx_cola_ready_prioritaria);
        recibir_pcb_cpu_FIFO();
    }
}

void algoritmo_FIFO(t_queue* queue, pthread_mutex_t* semaforo) {
    t_pcb* pcb = tomar_primero(queue, semaforo);
    actualizar_estado(pcb, EXEC);
    enviar_pcb(pcb, PCB, conexion_cpu_dispatch);
    eliminar_pcb(pcb);
}

void recibir_pcb_cpu_FIFO() {
    op_code cod_op = recibir_operacion(conexion_cpu_dispatch);
    t_pcb* pcb;
    t_solicitud_io* solicitud;
    t_paquete* paquete;
    switch (cod_op) {
        case PCB_EXIT:
            terminar_proceso(PCB_EXIT);
            break;
        case PCB_BLOCK:
            solicitud = recibir_pcb_io(conexion_cpu_dispatch);
            manejar_bloqueo(solicitud);
            break;
        case PAGE_FAULT:
            manejar_page_fault();
            // pthread_t t_pf;
            // pthread_create(&t_pf, NULL, (void*)manejar_page_fault, NULL);
            // pthread_detach(t_pf);
            break;
        case SEGMENTATION_FAULT:
            terminar_proceso(SEGMENTATION_FAULT);
            break;
        default:
            pthread_mutex_lock(&mx_log);
            log_warning(logger, "Operacion desconocida.");
            pthread_mutex_unlock(&mx_log);
            break;
    }
}

//--------RR------------

void planificador_corto_RR() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        algoritmo_RR(cola_ready_prioritaria, &mx_cola_ready_prioritaria);
        recibir_pcb_cpu_RR();
    }
}

void algoritmo_RR(t_queue* queue, pthread_mutex_t* semaforo) {
    t_pcb* pcb = tomar_primero(queue, semaforo);
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
    t_paquete* paquete;
    switch (cod_op) {
        case PCB_EXIT:
            pthread_cancel(t_quantum);
            terminar_proceso(PCB_EXIT);
            break;
        case PCB_BLOCK:
            pthread_cancel(t_quantum);
            solicitud = recibir_pcb_io(conexion_cpu_dispatch);
            manejar_bloqueo(solicitud);
            break;
        case INTERRUPCION:
            pcb = recibir_pcb(conexion_cpu_dispatch);
            actualizar_estado(pcb, READY);
            pthread_mutex_lock(&mx_log);
            log_info(logger, "PID: <%d> - Desalojado por fin de Quantum", pcb->pid);
            pthread_mutex_unlock(&mx_log);
            es_algoritmo_FEEDBACK() ? pushear_semaforizado(cola_ready_segundo_nivel, pcb, &mx_cola_ready_segunda)
                                    : pushear_semaforizado(cola_ready_prioritaria, pcb, &mx_cola_ready_prioritaria);
            loggear_colas_ready();
            sem_post(&sem_procesos_ready);  // hago el post despues para que muestre bien las colas ready
            break;
        case PAGE_FAULT:
            pthread_cancel(t_quantum);
            manejar_page_fault();
            break;
        case SEGMENTATION_FAULT:
            pthread_cancel(t_quantum);
            terminar_proceso(SEGMENTATION_FAULT);
            break;
        default:
            pthread_mutex_lock(&mx_log);
            log_warning(logger, "Operacion desconocida.");
            pthread_mutex_unlock(&mx_log);
            break;
    }
}

void esperar_quantum() {
    usleep(config_valores.quantum_rr * 1000);
    int interrupcion = 1;
    send(conexion_cpu_interrupt, &interrupcion, sizeof(uint32_t), MSG_WAITALL);
}

//--------------------FEEDBACK--------------------------------
void planificador_corto_FEEDBACK() {
    while (1) {
        sem_wait(&sem_procesos_ready);
        if (queue_size(cola_ready_prioritaria) > 0) {
            algoritmo_RR(cola_ready_prioritaria, &mx_cola_ready_prioritaria);
            recibir_pcb_cpu_RR();
        } else {
            algoritmo_FIFO(cola_ready_segundo_nivel, &mx_cola_ready_segunda);
            recibir_pcb_cpu_FIFO();
        }
    }
}

//-----------------GENERALES----------------------
void* tomar_primero(t_queue* queue, pthread_mutex_t* semaforo) {
    void* dato;
    pthread_mutex_lock(semaforo);
    dato = queue_pop(queue);
    pthread_mutex_unlock(semaforo);
    return dato;
}

void manejar_bloqueo(t_solicitud_io* solicitud) {
    actualizar_estado(solicitud->pcb, BLOCKED);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "PID: <%d> - Bloqueado por: <%s>", solicitud->pcb->pid, solicitud->dispositivo);
    pthread_mutex_unlock(&mx_log);
    if (strcmp(solicitud->dispositivo, "PANTALLA") == 0) {
        pthread_t t_bloqueo;
        pthread_create(&t_bloqueo, NULL, (void*)io_pantalla, (void*)solicitud);
        pthread_detach(t_bloqueo);
    } else if (strcmp(solicitud->dispositivo, "TECLADO") == 0) {
        pthread_t t_bloqueo;
        pthread_create(&t_bloqueo, NULL, (void*)io_teclado, (void*)solicitud);
        pthread_detach(t_bloqueo);
    } else {
        bool _coincide_nombre_cola(t_cola_bloqueo * una_cola) {
            return strcmp(solicitud->dispositivo, una_cola->dispositivo) == 0;
        }
        t_cola_bloqueo* cola = list_find(lista_colas_bloqueo, (void*)_coincide_nombre_cola);

        pushear_semaforizado(cola->cola_bloqueados, solicitud, &(cola->mx_cola_bloqueados));
        sem_post(&(cola->procesos_bloqueado));
    }
}

void manejar_page_fault() {
    t_pf_request* pf = malloc(sizeof(t_pf_request));
    int size, desplazamiento = 0;
    void* buffer;
    buffer = recibir_buffer(&size, conexion_cpu_dispatch);
    memcpy(&(pf->numero_seg), buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(&(pf->numero_pag), buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    t_pcb* pcb = deserializar_pcb(buffer, &desplazamiento);
    pf->pcb = pcb;

    actualizar_estado(pcb, BLOCKED);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>", pcb->pid, pf->numero_seg, pf->numero_pag);
    pthread_mutex_unlock(&mx_log);

    pthread_t t_page_fault;
    pthread_create(&t_page_fault, NULL, (void*)esperar_carga_pagina, pf);
    pthread_detach(t_page_fault);
    free(buffer);
}

void esperar_carga_pagina(t_pf_request* page_fault_request) {
    t_paquete* paquete = crear_paquete(PAGE_FAULT);
    t_segmento* entrada = list_get(page_fault_request->pcb->tabla_segmentos, page_fault_request->numero_seg);
    agregar_a_paquete(paquete, &(page_fault_request->pcb->pid), sizeof(u_int32_t));
    agregar_a_paquete(paquete, &(entrada->indice_tabla_paginas), sizeof(u_int32_t));
    agregar_a_paquete(paquete, &(page_fault_request->numero_seg), sizeof(u_int32_t));
    agregar_a_paquete(paquete, &(page_fault_request->numero_pag), sizeof(u_int32_t));
    pthread_mutex_lock(&mx_memoria);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
    int respuesta;
    recv(conexion_memoria, &respuesta, sizeof(int), MSG_WAITALL);
    pthread_mutex_unlock(&mx_memoria);
    actualizar_estado(page_fault_request->pcb, READY);
    pushear_semaforizado(cola_ready_prioritaria, page_fault_request->pcb, &mx_cola_ready_prioritaria);
    loggear_colas_ready();
    sem_post(&sem_procesos_ready);
    free(page_fault_request);
}

void terminar_proceso(op_code caso) {
    int respuesta;
    t_pcb* pcb = recibir_pcb(conexion_cpu_dispatch);
    actualizar_estado(pcb, EXIT);
    t_paquete* paquete = crear_paquete(PCB_EXIT);
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(u_int32_t));
    for (int i = 0; i < list_size(pcb->tabla_segmentos); i++) {
        t_segmento* entrada = list_get(pcb->tabla_segmentos, i);
        agregar_a_paquete(paquete, &(entrada->indice_tabla_paginas), sizeof(u_int32_t));
    }

    pthread_mutex_lock(&mx_memoria);
    enviar_paquete(paquete, conexion_memoria);
    recv(conexion_memoria, &respuesta, sizeof(u_int32_t), MSG_WAITALL);
    pthread_mutex_unlock(&mx_memoria);
    enviar_paquete(paquete, conexion_cpu_dispatch);
    recv(conexion_cpu_dispatch, &respuesta, sizeof(u_int32_t), MSG_WAITALL);
    eliminar_paquete(paquete);

    paquete = crear_paquete(caso);
    enviar_paquete(paquete, pcb->socket_consola);
    eliminar_pcb(pcb);
    eliminar_paquete(paquete);
    sem_post(&sem_grado_multiprogramacion);
}

void io_teclado(t_solicitud_io* solicitud) {
    int respuesta;
    op_code codigo = IO_TECLADO;
    send(solicitud->pcb->socket_consola, &codigo, sizeof(uint32_t), MSG_WAITALL);
    recv(solicitud->pcb->socket_consola, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    solicitud->pcb->registro[indice_registro(solicitud->parametro)] = respuesta;
    pushear_semaforizado(cola_ready_prioritaria, solicitud->pcb, &mx_cola_ready_prioritaria);
    actualizar_estado(solicitud->pcb, READY);
    loggear_colas_ready();
    sem_post(&sem_procesos_ready);
    liberar_solicitud(solicitud);
}

void io_pantalla(t_solicitud_io* solicitud) {
    int respuesta;
    t_paquete* paquete = crear_paquete(IO_PANTALLA);
    int dato = solicitud->pcb->registro[indice_registro(solicitud->parametro)];
    agregar_a_paquete(paquete, &dato, sizeof(int));
    enviar_paquete(paquete, solicitud->pcb->socket_consola);
    eliminar_paquete(paquete);
    recv(solicitud->pcb->socket_consola, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    pushear_semaforizado(cola_ready_prioritaria, solicitud->pcb, &mx_cola_ready_prioritaria);
    actualizar_estado(solicitud->pcb, READY);
    loggear_colas_ready();
    sem_post(&sem_procesos_ready);
    liberar_solicitud(solicitud);
}

void io_otros_dispositivos(t_cola_bloqueo* cola_bloqueo) {
    while (1) {
        sem_wait(&(cola_bloqueo->procesos_bloqueado));
        t_solicitud_io* solicitud = (t_solicitud_io*)tomar_primero(cola_bloqueo->cola_bloqueados, &(cola_bloqueo->mx_cola_bloqueados));
        int tiempo = atoi(solicitud->parametro) * cola_bloqueo->tiempo_dispositivo * 1000;
        usleep(tiempo);
        actualizar_estado(solicitud->pcb, READY);
        pushear_semaforizado(cola_ready_prioritaria, solicitud->pcb, &mx_cola_ready_prioritaria);
        loggear_colas_ready();
        sem_post(&sem_procesos_ready);
        liberar_solicitud(solicitud);
    }
}
