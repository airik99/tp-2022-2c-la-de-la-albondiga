#include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;
char* ip;
t_pcb* pcb_recibido;
int conexion_memoria, parar_proceso, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t conexion_memoria_i, hilo_dispatch, hilo_interrupt, pedidofin;
int ultimo_pid = -1;  // no se para que es esto pero se usa
int registros[] = {0, 0, 0, 0};

int main(int argc, char** argv) {
    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    ip = "127.0.0.1";  // esto no debería estar hardcodeado
    cargar_configuracion();

    // CONEXION CON MEMORIA
    // log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria \n");

    // conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);
    // error_conexion(conexion_memoria);

    // pthread_create(&conexion_memoria_i, NULL, conexion_inicial_memoria, NULL);
    // log_info(logger, "Conexion con memoria exitosa \n");

    // log_info(logger, "Hilo de conexion con memoria creado \n");

    log_info(logger, "Iniciando conexion con kernel por interrupt\n");
    socket_servidor_interrupt = iniciar_servidor(ip, config_valores.puerto_escucha_interrupt);
	log_info(logger, "Esperando cliente por Interrupt");
    int socket_kernel = esperar_cliente(socket_servidor_interrupt);
    log_info(logger, "Conexión con Kernel en puerto Interrupt establecida.");

    log_info(logger, "Iniciando conexion con kernel por dispatch\n");
    socket_servidor_dispatch = iniciar_servidor(ip, config_valores.puerto_escucha_dispatch);
	log_info(logger, "Esperando cliente por dispatch");
    cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);
    log_info(logger, "Conexión con Kernel en puerto Dispatch establecida.");

    //error_conexion(socket_servidor_dispatch);

    pthread_create(&hilo_interrupt, NULL, (void*)esperar_kernel_interrupt, (void*)socket_servidor_interrupt);
    pthread_create(&hilo_dispatch, NULL, (void*)esperar_kernel_dispatch, (void*)socket_servidor_dispatch);
    log_info(logger, "Hilo de interrupciones creado \n");
    log_info(logger, "Hilo dispatch creado\n");

    pthread_join(hilo_dispatch, NULL);
    pthread_join(hilo_interrupt, NULL);

    // CONEXION CON KERNEL

    liberar_todo();
    return EXIT_SUCCESS;
}

void esperar_kernel_dispatch(int socket_servidor_dispatch) {
    while (1) {
        op_code operacion = recibir_operacion(cliente_servidor_dispatch);
        evaluar_cod_op(operacion);
    }
}

void esperar_kernel_interrupt(int socket_servidor_interrupt) {
}

void error_conexion(int socket) {
    if (socket == -1) {
        log_info(logger, "Error en la conexion al servidor.\n");
        exit(EXIT_FAILURE);
    }
}

void evaluar_cod_op(int cod_op) {
    switch (cod_op) {
        case PCB:
            parar_proceso = 0;                                      // INICIA EL CONTADOR DE PARAR PROCESO
            pcb_recibido = recibir_pcb(cliente_servidor_dispatch);  
            log_info(logger, "Recibi PCB de Id: %d \n", pcb_recibido->pid);
            /*if(ultimo_pid != pcb_recibido->pid && list_size(tlb->lista) != 0){
                    //vaciarTlb();
                    log_info(logger,"Se vacio TLB\n");
            }*/
            //ciclo_de_instruccion(pcb_recibido);  // INICIA EL CICLO DE INSTRUCCION
			t_paquete* retorno_pcb = crear_paquete(PCB_EXIT);
			sleep(2);
			serializar_pcb(retorno_pcb, pcb_recibido);
			enviar_paquete(retorno_pcb, cliente_servidor_dispatch);
            break;
            /* case HANDSHAKE:
                 log_info(logger, "Se recibio la configuracion por handshake \n");
                 t_handshake* configuracion_tabla = recibir_handshake(conexion_memoria);
                 return NULL;
                 break;
             case INTERRUPCION:
                 log_info(logger, "Peticion de interrupcion recibida \n");
                 parar_proceso++;
                 break;
                 casos_de_error(cod_op);*/
    }
    return EXIT_SUCCESS;
}

void casos_de_error(int codigo_error) {
    switch (codigo_error) {
        case -1:
            log_error(logger, "Fallo la comunicacion. Abortando \n");
            return EXIT_FAILURE;
            break;
        default:
            log_warning(logger, "Operacion desconocida \n");
            break;
    }
}

void cargar_configuracion() {
    config = config_create("cfg/archivo_configuracion.config");
    log_info(logger, "Arranco a leer el archivo de configuracion \n");

    config_valores.entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    config_valores.reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
    config_valores.retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    config_valores.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

    log_info(logger, "Termino de leer el archivo de configuracion \n");
}

void liberar_todo() {
    liberar_conexion(conexion_memoria);
    liberar_conexion(socket_servidor_dispatch);
    liberar_conexion(cliente_servidor_dispatch);
    config_destroy(config);
    log_destroy(logger);
}

/*
/*
void* ciclo_de_instruccion(t_pcb* pcb) {
    t_instruccion* instruccionProxima;
    parar_proceso = 0;

    log_info(logger, "Se creo hilo para recibir interrupciones \n");

    while (pcb->program_counter < list_size(pcb->instrucciones)) {
        instruccionProxima = list_get(pcb->instrucciones, pcb->program_counter);  // FETCH
        decode(instruccionProxima, pcb);                                          // DECODE (CON EXECUTE INCLUIDO)
        pcb->program_counter++;

        if (checkInterrupt() == 1) {  // SE FIJA QUE NO HAYA PEDIDO DE PARAR EL PROCESO ANTES DE SEGUIR CON EL CICLO DE INSTRUCCION
            ultimo_pid = pcb->pid;
            // enviarPcb(socket_kernel, pcb);
            log_info(logger, "Envio pcb devuelta al kernel \n");
            return NULL;
        }
    }
    return NULL;
}

void decode(t_instruccion* instruccion, t_pcb* pcb) {
    if (strcmp(instruccion->nombre, "SET") == 0) {
        registro_cpu registro = list_get(instruccion->params, 0);
        uint32_t valor = list_get(instruccion->params, 1);
        ejecutar_SET(registro, valor);
    }

    if (strcmp(instruccion->nombre, "ADD") == 0) {
        registro_cpu destino = list_get(instruccion->params, 0);
        registro_cpu origen = list_get(instruccion->params, 1);
        ejecutar_ADD(destino, origen);
    }

    if (strcmp(instruccion->nombre, "MOV_IN") == 0) {
        // ejecutar_MOV_IN();
    }

    if (strcmp(instruccion->nombre, "MOV_OUT") == 0) {
        // ejecutar_MOV_OUT();
    }

    if (strcmp(instruccion->nombre, "I/O") == 0) {
        // ejecutar_IO();
    }

    if (strcmp(instruccion->nombre, "EXIT") == 0) {
        ejecutar_EXIT(pcb);
    }
}

uint32_t AX, BX, CX, DX;  // esto no se usa
registros[] = {0, 0, 0, 0};

/*SET (Registro, Valor): Asigna al registro el valor pasado como parámetro.
void ejecutar_SET(registro_cpu registro, uint32_t valor) {
    log_info(logger, "Nos llego el valor %d \n", valor);
    registros[registro] = valor;
    log_info("Guardamos el valor %d en el registro %d \n", valor, registro);  // hay que ver si devuelve un numero o el enum en sí
}

//ADD (Registro Destino, Registro Origen): Suma ambos registros y deja el resultado en el Registro Destino.
void ejecutar_ADD(registro_cpu destino, registro_cpu origen) {
    registros[destino] = registros[destino] + registros[origen];
    log_info("La suma entre %d y %d es %d \n", destino, origen, registros[destino]);
}

//I/O (Dispositivo, Registro / Unidades de trabajo): Esta instrucción representa una syscall de I/O bloqueante. Se deberá devolver
//el Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea
//utilizar el proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).
void ejecutar_IO(t_pcb* pcb, char* dispositivo_a_asignar, t_instruccion* instruccion /*, int unidades_a_asignar) {
    t_pcb* pcb_a_enviar = pcb;
    pcb_a_enviar->tiempo_bloqueo = atoi(list_get(instruccion->params, 1));  // CHEQUEAR QUE FUNCIONE
    actualizar_estado(pcb_a_enviar, BLOCKED);
    // enviar_mensaje(kernel,pcb_a_enviar);
}

//EXIT Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización.
void ejecutar_EXIT(t_pcb* pcb) {
    pcb->estado_actual = EXIT;
    pthread_mutex_lock(&pedidofin);
    parar_proceso++;
    pthread_mutex_unlock(&pedidofin);
    log_info(logger, "Se ejecuto instruccion EXIT \n");
}

int checkInterrupt() {  // chequear esto TODO
    pthread_mutex_lock(&pedidofin);
    if (parar_proceso > 0) {
        pthread_mutex_unlock(&pedidofin);
        return 1;
    } else {
        pthread_mutex_unlock(&pedidofin);
        return 0;
    }
}

void* conexion_inicial_memoria() {
    pedir_handshake(conexion_memoria);
    log_info(logger, "Se envio el pedido de handshake\n");

    while (1) {
        int cod_op = recibir_operacion(conexion_memoria);
        evaluar_cod_op(cod_op);
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

void* conexion_interrupciones(char* ip, char* puerto) {
    int server_fd = iniciar_servidor(ip, puerto);
    log_info(logger, "CPU listo para recibir interrupciones \n");
    int cliente_fd = esperar_cliente(server_fd);
    log_info(logger, "Se conecto Kernel al puerto interrupt \n");

    while (1) {
        int cod_op;
        recibir_datos(cliente_fd, &cod_op, sizeof(int));
        evaluar_cod_op(cod_op);
    }
    return NULL;
}*/