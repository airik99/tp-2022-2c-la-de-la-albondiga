#include "cpu.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        puts("FALTO CONFIG");
        return EXIT_FAILURE;
    }
    char* path_config = argv[1];
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    cargar_configuracion(path_config);
    pthread_mutex_init(&mx_interrupcion, NULL);
    pthread_mutex_init(&mx_log, NULL);

    inicializar_tlb();

    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);
    error_conexion(conexion_memoria);
    int id_handshake = 0;
    send(conexion_memoria, &id_handshake, sizeof(int), MSG_WAITALL);
    log_info(logger, "Conexion con memoria exitosa \n");
    op_code operacion = recibir_operacion(conexion_memoria);
    t_list* lista = recibir_lista(conexion_memoria);
    cant_entradas_por_tabla = list_get(lista, 0);
    tam_pagina = list_get(lista, 1);
    list_destroy(lista);

    socket_servidor_interrupt = iniciar_servidor(config_valores.puerto_escucha_interrupt);
    log_info(logger, "Esperando cliente por Interrupt\n");
    cliente_servidor_interrupt = esperar_cliente(socket_servidor_interrupt);
    log_info(logger, "Conexión con Kernel en puerto Interrupt establecida.\n");

    socket_servidor_dispatch = iniciar_servidor(config_valores.puerto_escucha_dispatch);
    log_info(logger, "Esperando cliente por dispatch\n");
    cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);
    log_info(logger, "Conexión con Kernel en puerto Dispatch establecida.\n");

    pthread_create(&hilo_interrupt, NULL, (void*)esperar_kernel_interrupt, NULL);
    pthread_create(&hilo_dispatch, NULL, (void*)esperar_kernel_dispatch, NULL);

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    liberar_todo();
    return EXIT_SUCCESS;
}

void esperar_kernel_dispatch() {
    t_pcb* pcb_recibido;
    while (1) {
        op_code operacion = recibir_operacion(cliente_servidor_dispatch);
        switch (operacion) {
            case PCB:
                pcb_recibido = recibir_pcb(cliente_servidor_dispatch);
                copiar_valores_registros((pcb_recibido->registro), registros);
                ciclo_de_instruccion(pcb_recibido);
                eliminar_pcb(pcb_recibido);
                break;
            default:
                pthread_mutex_lock(&mx_log);
                log_error(logger, "Se desconecto kernel. Cerrando server");
                pthread_mutex_unlock(&mx_log);
                return;
        }
    }
}

void esperar_kernel_interrupt() {
    while (1) {
        int temp;
        recv(cliente_servidor_interrupt, &temp, sizeof(uint32_t), MSG_WAITALL);
        pthread_mutex_lock(&mx_interrupcion);
        interrupcion = temp;
        if (interrupcion != 1) {
            pthread_mutex_lock(&mx_log);
            log_error(logger, "Se desconecto kernel. Cerrando server");
            pthread_mutex_unlock(&mx_log);
            pthread_mutex_unlock(&mx_interrupcion);
            return;
        }
        pthread_mutex_unlock(&mx_interrupcion);
    }
}

void error_conexion(int socket) {
    if (socket == -1) {
        log_info(logger, "Error en la conexion al servidor.\n");
        exit(EXIT_FAILURE);
    }
}
