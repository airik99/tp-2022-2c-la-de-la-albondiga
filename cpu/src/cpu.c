#include "cpu.h"

int main(int argc, char** argv) {
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    cargar_configuracion();
    t_pcb* pcb_actual = malloc(sizeof(t_pcb));

    log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria \n");

    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);
    error_conexion(conexion_memoria);
    // pthread_create(&conexion_memoria_i, NULL, conexion_inicial_memoria, NULL);
    // log_info(logger, "Conexion con memoria exitosa \n");
    // log_info(logger, "Hilo de conexion con memoria creado \n");

    pedir_handshake();

    log_info(logger, "Iniciando conexion con kernel por interrupt\n");
    socket_servidor_interrupt = iniciar_servidor(config_valores.puerto_escucha_interrupt);
    log_info(logger, "Esperando cliente por Interrupt\n");
    cliente_servidor_interrupt = esperar_cliente(socket_servidor_interrupt);
    log_info(logger, "Conexión con Kernel en puerto Interrupt establecida.\n");

    log_info(logger, "Iniciando conexion con kernel por dispatch\n");
    socket_servidor_dispatch = iniciar_servidor(config_valores.puerto_escucha_dispatch);
    log_info(logger, "Esperando cliente por dispatch\n");
    cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);
    log_info(logger, "Conexión con Kernel en puerto Dispatch establecida.\n");

    // error_conexion(socket_servidor_dispatch);

    pthread_create(&hilo_interrupt, NULL, (void*)esperar_kernel_interrupt, NULL);
    pthread_create(&hilo_dispatch, NULL, (void*)esperar_kernel_dispatch, NULL);
    log_info(logger, "Hilo de interrupciones creado \n");
    log_info(logger, "Hilo dispatch creado\n");

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    liberar_todo();
    return EXIT_SUCCESS;
}

void pedir_handshake() {
    tam_pagina = pedir_configuracion_a_memoria(TAM_PAGINA);
	cant_entradas_por_tabla = pedir_configuracion_a_memoria(ENTRADAS_POR_TABLA);

    if(tam_pagina == -1 || cant_entradas_por_tabla == -1) {
        log_error(logger, "No se pudo obtener la configuracion de memoria \n");
        exit(1);
    } else {
        log_info(logger, "CPU recibio el handshake de memoria: tamanio de pagina: %d | cantidad de entradas por tabla: %d \n", tam_pagina, cant_entradas_por_tabla);
    }
}

uint32_t pedir_configuracion_a_memoria(int cod_op) {
    uint32_t respuesta;
	t_paquete* paquete = crear_paquete(cod_op);
	enviar_paquete(paquete, conexion_memoria);
    recv(conexion_memoria, &respuesta, sizeof(uint32_t), MSG_WAITALL);
    eliminar_paquete(paquete);
    return respuesta;
}

void esperar_kernel_dispatch() { //OJO QUE ESTO ES DE KERNEL, NO DE MEMORIA
    t_pcb* pcb_recibido;
    while (1) {
        op_code operacion = recibir_operacion(cliente_servidor_dispatch);
        switch (operacion) {
            case PCB:
                pcb_recibido = recibir_pcb(cliente_servidor_dispatch);
                copiar_valores_registros((pcb_recibido->registro), registros);
                
                if(pcb_actual->pid != pcb_recibido->pid && list_size(tlb->traducciones) != 0){
        		    vaciar_tlb();
        		    log_info(logger,"Se vacio TLB\n");
        	    }
                ciclo_de_instruccion(pcb_recibido);  // INICIA EL CICLO DE INSTRUCCION
                eliminar_pcb(pcb_recibido);
                break;
            default:
                log_error(logger, "Fallo la comunicacion con kernel. Abortando\n");
                finalizar();
                break;
        }
    }
}

void esperar_kernel_interrupt() {
    while (1) {
        recv(cliente_servidor_interrupt, &interrupcion, sizeof(uint32_t), MSG_WAITALL);
        if (interrupcion != 1) {
            log_error(logger, "Operacion desconocida por interrupt\n");
            finalizar();
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