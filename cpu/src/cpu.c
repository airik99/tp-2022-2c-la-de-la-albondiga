#include "cpu.h"

int main(int argc, char** argv) {
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    cargar_configuracion();

    // CONEXION CON MEMORIA
    log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria \n");

    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);
    error_conexion(conexion_memoria);
    // pthread_create(&conexion_memoria_i, NULL, conexion_inicial_memoria, NULL);
    log_info(logger, "Conexion con memoria exitosa \n");
    log_info(logger, "Hilo de conexion con memoria creado \n");

    log_info(logger, "Iniciando conexion con kernel por interrupt\n");
    socket_servidor_interrupt = iniciar_servidor(config_valores.puerto_escucha_interrupt);
    log_info(logger, "Esperando cliente por Interrupt");
    cliente_servidor_interrupt = esperar_cliente(socket_servidor_interrupt);
    log_info(logger, "Conexión con Kernel en puerto Interrupt establecida.");

    log_info(logger, "Iniciando conexion con kernel por dispatch\n");
    socket_servidor_dispatch = iniciar_servidor(config_valores.puerto_escucha_dispatch);
    log_info(logger, "Esperando cliente por dispatch");
    cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);
    log_info(logger, "Conexión con Kernel en puerto Dispatch establecida.");

    // error_conexion(socket_servidor_dispatch);

    pthread_create(&hilo_interrupt, NULL, (void*)esperar_kernel_interrupt, NULL);
    pthread_create(&hilo_dispatch, NULL, (void*)esperar_kernel_dispatch, NULL);
    log_info(logger, "Hilo de interrupciones creado \n");
    log_info(logger, "Hilo dispatch creado\n");

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    // CONEXION CON KERNEL

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
                ciclo_de_instruccion(pcb_recibido);  // INICIA EL CICLO DE INSTRUCCION
                eliminar_pcb(pcb_recibido);
                break;
            case HANDSHAKE:
                log_info(logger, "Se recibio la configuracion por handshake");
                t_handshake* configuracion_tabla = recibir_handshake(conexion_memoria);
                break;
            default:
                log_error(logger, "Fallo la comunicacion con kernel. Abortando");
                finalizar();
                break;
        }
    }
}

void esperar_kernel_interrupt() {
    while (1) {
        recv(cliente_servidor_interrupt, &interrupcion, sizeof(uint32_t), MSG_WAITALL);
        if (interrupcion != 1) {
            log_error(logger, "Operacion desconocida por interrupt");
            finalizar();
        }
        log_info(logger, "Recibi interrupt");
    }
}

void error_conexion(int socket) {
    if (socket == -1) {
        log_info(logger, "Error en la conexion al servidor.");
        exit(EXIT_FAILURE);
    }
}

int leer_de_memoria(t_direccion_logica* direccion_logica) {
    // TODO
    return 0;
}

char* traducir_direccion_logica(char* direccion_logica) {
    char* direccion_fisica;
    return direccion_fisica;
}

void escribir_en_memoria(char* direccion_fisica, int valor) {
    // int indice = indice_registro(direccion_fisica);
    // memoria[indice] = valor;
    // log_info(logger, "Se guarda el valor %d en la direccion %s \n", valor, direccion_fisica);  // hay que ver si devuelve un numero o el enum en sí
}

void* conexion_inicial_memoria() {
    pedir_handshake(conexion_memoria);
    log_info(logger, "Se envio el pedido de handshake\n");
    int cod_op;
    while (1) {
        cod_op = recibir_operacion(conexion_memoria);
    }
    return NULL;
}

void pedir_handshake(int socket_memoria) {
    op_code codigo = HANDSHAKE;
    enviar_datos(socket_memoria, &codigo, sizeof(op_code));
}

t_handshake* recibir_handshake(int socket_memoria) {
    t_handshake* han = malloc(sizeof(t_handshake));
    t_list* valores;
    // valores = recibir_paquete(socket_memoria); //TODO LA FUNCION DE RECIBIR PAQUETE NO ESTA HECHA
    han->tam_pagina = list_get(valores, 0);
    han->entradas = list_get(valores, 1);

    list_destroy(valores);

    return han;
}
