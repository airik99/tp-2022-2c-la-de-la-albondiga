#include <ciclo_instruccion.h>

void ciclo_de_instruccion(t_pcb* pcb) {
    t_instruccion* instruccion;
    flag_salida = 0;
    pthread_mutex_lock(&mx_interrupcion);
    interrupcion = 0;
    pthread_mutex_unlock(&mx_interrupcion);
    pcb_actual = pcb;

    while (pcb->program_counter < list_size(pcb->instrucciones)) {
        instruccion = list_get(pcb->instrucciones, pcb->program_counter);  // FETCH
        decode(instruccion, pcb);                                          // DECODE (CON EXECUTE INCLUIDO)
        if (flag_salida) {
            return;
        }

        if (check_interrupt()) {  // SE FIJA QUE NO HAYA UNA INTERRUPCION ANTES DE SEGUIR CON EL CICLO DE INSTRUCCION
            t_paquete* paquete = crear_paquete(INTERRUPCION);
            copiar_valores_registros(registros, (pcb->registro));
            pcb_actual->program_counter++;
            serializar_pcb(paquete, pcb);
            enviar_paquete(paquete, cliente_servidor_dispatch);
            eliminar_paquete(paquete);
            return;
        }
    }
    eliminar_pcb(pcb);
}

void decode(t_instruccion* instruccion, t_pcb* pcb) {
    if (strcmp(instruccion->nombre, "SET") == 0) {
        char* registro = list_get(instruccion->params, 0);
        uint32_t valor = atoi(list_get(instruccion->params, 1));
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <SET> - <%s> - <%d>\n", pcb->pid, registro, valor);  // log obligatorio
        pthread_mutex_unlock(&mx_log);
        ejecutar_SET(registro, valor);
    }

    else if (strcmp(instruccion->nombre, "ADD") == 0) {
        char* destino = list_get(instruccion->params, 0);
        char* origen = list_get(instruccion->params, 1);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <ADD> - <%s> - <%s>\n", pcb->pid, destino, origen);  // log obligatorio
        pthread_mutex_unlock(&mx_log);
        ejecutar_ADD(destino, origen);
    }

    else if (strcmp(instruccion->nombre, "MOV_IN") == 0) {
        int direccion_logica = atoi(list_get(instruccion->params, 1));
        char* registro = list_get(instruccion->params, 0);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_IN> - <%d> - <%s>\n", pcb->pid, direccion_logica, registro);  // log obligatorio
        pthread_mutex_unlock(&mx_log);
        ejecutar_MOV_IN(registro, direccion_logica);
    }

    else if (strcmp(instruccion->nombre, "MOV_OUT") == 0) {
        uint32_t direccion_logica = atoi(list_get(instruccion->params, 0));
        char* registro = list_get(instruccion->params, 1);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_OUT> - <%d> - <%s>\n", pcb->pid, direccion_logica, registro);  // log obligatorio
        pthread_mutex_unlock(&mx_log);
        ejecutar_MOV_OUT(direccion_logica, registro);
    }

    else if (strcmp(instruccion->nombre, "I/O") == 0) {
        char* dispositivo = list_get(instruccion->params, 0);
        char* param2 = list_get(instruccion->params, 1);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <I/O> - <%s> - <%s>\n", pcb->pid, dispositivo, param2);  // log obligatorio
        pthread_mutex_unlock(&mx_log);
        ejecutar_IO(dispositivo, param2, pcb);
    }

    else if (strcmp(instruccion->nombre, "EXIT") == 0) {
        pthread_mutex_lock(&mx_log);
        log_info(logger, "PID: <%d> - Ejecutando: <EXIT> -\n", pcb->pid);
        pthread_mutex_unlock(&mx_log);
        ejecutar_EXIT(pcb);
    } else {
        pthread_mutex_lock(&mx_log);
        log_error(logger, "PID: <%d> - INSTRUCCION NO RECONOCIDA\n", pcb->pid);
        pthread_mutex_unlock(&mx_log);
    }
}

void ejecutar_SET(char* registro, uint32_t valor) {
    usleep(config_valores.retardo_instruccion * 1000);
    int indice = indice_registro(registro);
    registros[indice] = valor;
    pcb_actual->program_counter++;
}

void ejecutar_ADD(char* destino, char* origen) {
    usleep(config_valores.retardo_instruccion * 1000);
    int registro_origen = indice_registro(origen);
    int registro_destino = indice_registro(destino);
    int resultado = registros[registro_destino] + registros[registro_origen];
    registros[registro_destino] = resultado;
    pcb_actual->program_counter++;
}

void ejecutar_IO(char* dispositivo, char* parametro, t_pcb* pcb) {
    t_paquete* paquete = crear_paquete(PCB_BLOCK);
    int largo_nombre = strlen(dispositivo) + 1;
    int largo_parametro = strlen(parametro) + 1;
    copiar_valores_registros(registros, (pcb->registro));
    pcb_actual->program_counter++;
    serializar_pcb(paquete, pcb);
    agregar_a_paquete(paquete, &largo_nombre, sizeof(int));
    agregar_a_paquete(paquete, dispositivo, largo_nombre);
    agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
    agregar_a_paquete(paquete, parametro, largo_parametro);
    enviar_paquete(paquete, cliente_servidor_dispatch);
    eliminar_paquete(paquete);
    flag_salida = 1;
}

void ejecutar_MOV_IN(char* registro, uint32_t direccion_logica) {
    int direccion_fisica = traducir_direccion_logica(direccion_logica);
    if (direccion_fisica < 0) {
        flag_salida = 1;
        return;
    }
    uint32_t valor = leer_de_memoria(direccion_fisica);
    int indice = indice_registro(registro);
    registros[indice] = valor;
    pcb_actual->program_counter++;
}

uint32_t leer_de_memoria(int direccion_fisica) {
    uint32_t respuesta;
    t_paquete* paquete = crear_paquete(LEER_DE_MEMORIA);  // TODO: aca hay que acordarnos de evaluar este cod_op en memoria
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "PID: <%d> - Acción: <LEER> - Segmento:< %d > -Pagina : <%d> - Dirección Fisica: <%d>", pcb_actual->pid, num_segmento_actual, num_pagina_actual, direccion_fisica);
    pthread_mutex_unlock(&mx_log);

    eliminar_paquete(paquete);
    recv(conexion_memoria, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    return respuesta;
}

void ejecutar_MOV_OUT(uint32_t direccion_logica, char* registro) {
    int direccion_fisica = traducir_direccion_logica(direccion_logica);
    if (direccion_fisica < 0) {
        flag_salida = 1;
        return;
    }
    int indice = indice_registro(registro);
    uint32_t valor = registros[indice];
    escribir_en_memoria(direccion_fisica, valor);
    pcb_actual->program_counter++;
}

void escribir_en_memoria(int direccion_fisica, uint32_t valor) {
    pthread_mutex_lock(&mx_log);
    log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Segmento:< %d > -Pagina : <%d> - Dirección Fisica: <%d>", pcb_actual->pid, num_segmento_actual, num_pagina_actual, direccion_fisica);
    pthread_mutex_unlock(&mx_log);
    uint32_t respuesta;
    t_paquete* paquete = crear_paquete(ESCRIBIR_EN_MEMORIA);  // TODO: aca hay que acordarnos de evaluar este cod_op en memoria
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    recv(conexion_memoria, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    eliminar_paquete(paquete);
    if (respuesta == -1) {  // TODO: aca hay que acordarnos que la memoria nos tiene que devolver esto cuando no se pudo escribir
        pthread_mutex_lock(&mx_log);
        log_error(logger, "No se pudo escribir en memoria");
        pthread_mutex_unlock(&mx_log);
    }
}

void ejecutar_EXIT(t_pcb* pcb) {
    copiar_valores_registros(registros, (pcb->registro));
    enviar_pcb(pcb, PCB_EXIT, cliente_servidor_dispatch);
    flag_salida = 1;
}

int check_interrupt() {
    pthread_mutex_lock(&mx_interrupcion);
    int temp = interrupcion;
    pthread_mutex_unlock(&mx_interrupcion);
    return temp;
}