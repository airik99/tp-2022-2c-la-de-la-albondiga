#include "cpu.h"

int main(int argc, char** argv) {
    //tiempo = time(NULL);
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    cargar_configuracion();
    t_pcb* pcb_actual = malloc(sizeof(t_pcb)); //TODO: ver si esto esta bien

    // CONEXION CON MEMORIA
    // log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria \n");

    // conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);
    // error_conexion(conexion_memoria);

    // pthread_create(&conexion_memoria_i, NULL, conexion_inicial_memoria, NULL);
    // log_info(logger, "Conexion con memoria exitosa \n");

    // log_info(logger, "Hilo de conexion con memoria creado \n");
    pedir_configuracion_a_memoria(TAM_PAGINA);
	pedir_configuracion_a_memoria(ENTRADAS_POR_TABLA);

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

    // CONEXION CON KERNEL

    liberar_todo();
    return EXIT_SUCCESS;
}

int pedir_configuracion_a_memoria(int cod_op) {
	t_paquete* paquete = crear_paquete(cod_op);
	enviar_paquete(paquete, conexion_memoria);
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

void recibir_de_memoria() { 
    while (1) {
        op_code operacion = recibir_operacion(conexion_memoria);
        switch (operacion) {
            case HANDSHAKE:
                log_info(logger, "Se recibio la configuracion por handshake\n");
                t_handshake* configuracion_tabla = recibir_handshake(conexion_memoria);
                break;
            case TAM_PAGINA:
                log_info(logger, "Se recibe el tamaño de paginas\n");
                tam_pagina = recibir_numero(conexion_memoria);
                break;
            case ENTRADAS_POR_TABLA:
                log_info(logger, "Se recibe la cantidad de entradas por tabla\n");
                cant_entradas_por_tabla = recibir_numero(conexion_memoria);
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
