#include "cpu.h"

int main(int argc, char** argv) {
    if (argc < 2){
        puts("FALTO CONFIG");
        return EXIT_FAILURE;
    }
    char *path_config = argv[1];
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    cargar_configuracion(path_config);
    pthread_mutex_init(&mx_traduccion_direccion_logica, NULL);

    inicializar_tlb();
    
    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);
    error_conexion(conexion_memoria);
    // pthread_create(&t_conexion_memoria, NULL, conexion_inicial_memoria, NULL);
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
                log_error(logger, "Se desconecto kernel. Cerrando server");
                return;
        }
    }
}

void esperar_kernel_interrupt() {
    while (1) {
        recv(cliente_servidor_interrupt, &interrupcion, sizeof(uint32_t), MSG_WAITALL);
        if (interrupcion != 1) {
            log_error(logger, "Se desconecto kernel. Cerrando server");
            return;
        }
        log_info(logger, "Recibi interrupt\n");
    }
}

void error_conexion(int socket) {
    if (socket == -1) {
        log_info(logger, "Error en la conexion al servidor.\n");
        exit(EXIT_FAILURE);
    }
}
