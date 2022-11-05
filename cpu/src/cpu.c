#include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;
char* ip;
t_handshake* configuracion_tabla;
t_pcb* pcb_recibido;
int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t conexion_memoria_i, hilo_dispatch, hilo_interrupt, pedidofin;
int ultimo_pid = -1;
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
    cliente_servidor_interrupt = esperar_cliente(socket_servidor_interrupt);
    log_info(logger, "Conexión con Kernel en puerto Interrupt establecida.");

    log_info(logger, "Iniciando conexion con kernel por dispatch\n");
    socket_servidor_dispatch = iniciar_servidor(ip, config_valores.puerto_escucha_dispatch);
    log_info(logger, "Esperando cliente por dispatch");
    cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);
    log_info(logger, "Conexión con Kernel en puerto Dispatch establecida.");

    // error_conexion(socket_servidor_dispatch);

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
    while (1) {
        op_code operacion = recibir_operacion(cliente_servidor_interrupt);
        evaluar_cod_op(operacion);
    }
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
            parar_proceso = 0;  // INICIA EL CONTADOR DE PARAR PROCESO
            pcb_recibido = recibir_pcb(cliente_servidor_dispatch);
            copiar_registros_de_pcb(pcb_recibido);
            log_info(logger, "Recibi PCB de Id: %d \n", pcb_recibido->pid);
            /*if(ultimo_pid != pcb_recibido->pid && list_size(tlb->lista) != 0){
                    //vaciarTlb();
                    //log_info(logger,"Se vacio TLB\n");
            }*/
            ciclo_de_instruccion(pcb_recibido);  // INICIA EL CICLO DE INSTRUCCION
            break;
        case PAQUETE:
            log_info(logger, "Recibi configuracion por handshake \n");
            configuracion_tabla = recibir_handshake(conexion_memoria);
            break;
        case HANDSHAKE:
            log_info(logger, "Se recibio la configuracion por handshake \n");
            t_handshake* configuracion_tabla = recibir_handshake(conexion_memoria);
            break;
        case INTERRUPCION:
            log_info(logger, "Peticion de interrupcion recibida \n");
            parar_proceso++;
            break;
            casos_de_error(cod_op);
    }
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

void* ciclo_de_instruccion(t_pcb* pcb) {
    t_instruccion* instruccionProxima;
    parar_proceso = 0;

    while (pcb->program_counter < list_size(pcb->instrucciones)) {
        instruccionProxima = list_get(pcb->instrucciones, pcb->program_counter);  // FETCH
        decode(instruccionProxima, pcb);                                          // DECODE (CON EXECUTE INCLUIDO)
        pcb->program_counter++;

        /*if (checkInterrupt() == 1) {  // SE FIJA QUE NO HAYA PEDIDO DE PARAR EL PROCESO ANTES DE SEGUIR CON EL CICLO DE INSTRUCCION
            ultimo_pid = pcb->pid;
            // enviarPcb(socket_kernel, pcb);
            log_info(logger, "Envio pcb devuelta al kernel \n");
            return;
        }*/
    }
    t_paquete* retorno_pcb = crear_paquete(PCB_EXIT);
    serializar_pcb(retorno_pcb, pcb_recibido);
    enviar_paquete(retorno_pcb, cliente_servidor_dispatch);
}

void decode(t_instruccion* instruccion, t_pcb* pcb) {
    if (strcmp(instruccion->nombre, "SET") == 0) {
        char* registro = list_get(instruccion->params, 0);
        uint32_t valor = atoi(list_get(instruccion->params, 1));
        log_info(logger, "PID: <%d> - Ejecutando: <SET> - <%s> - <%d>", pcb->pid, registro, valor);
        ejecutar_SET(registro, valor);
    }

    if (strcmp(instruccion->nombre, "ADD") == 0) {
        char* destino = list_get(instruccion->params, 0);
        char* origen = list_get(instruccion->params, 1);
        log_info(logger, "PID: <%d> - Ejecutando: <ADD> - <%s> - <%s>", pcb->pid, destino, origen);
        ejecutar_ADD(destino, origen);
    }

    if (strcmp(instruccion->nombre, "MOV_IN") == 0) {
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_IN> - <PENDIENTE> - <PENDIENTE>", pcb->pid);
        // ejecutar_MOV_IN();
    }

    if (strcmp(instruccion->nombre, "MOV_OUT") == 0) {
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_OUT> - <PENDIENTE> - <PENDIENTE>", pcb->pid);
        // ejecutar_MOV_OUT();
    }

    if (strcmp(instruccion->nombre, "I/O") == 0) {
        char* dispositivo = list_get(instruccion->params, 0);
        char* param2 = list_get(instruccion->params, 1);
        log_info(logger, "PID: <%d> - Ejecutando: <I/O> - <%s> - <%s>", pcb->pid, dispositivo, param2);
        ejecutar_IO(dispositivo, param2);
    }

    if (strcmp(instruccion->nombre, "EXIT") == 0) {
        log_info(logger, "PID: <%d> - Ejecutando: <EXIT> -", pcb->pid);
        ejecutar_EXIT(pcb);
    }
}

// SET (Registro, Valor): Asigna al registro el valor pasado como parámetro.
void ejecutar_SET(char* registro, uint32_t valor) {
    int indice = indice_registro(registro);
    registros[indice] = valor;
    log_info("Guardamos el valor %d en el registro %s \n", valor, registro);  // hay que ver si devuelve un numero o el enum en sí
}

// ADD (Registro Destino, Registro Origen): Suma ambos registros y deja el resultado en el Registro Destino.
void ejecutar_ADD(char* destino, char* origen) {
    int registro_origen = indice_registro(origen);
    int registro_destino = indice_registro(destino);
    int resultado = registros[registro_destino] + registros[registro_origen];
    log_info(logger, "La suma entre %s (%d) y %s (%d) es %d \n", destino,registros[registro_destino], origen, registros[registro_origen], resultado);
    registros[registro_destino] = resultado;
}

// I/O (Dispositivo, Registro / Unidades de trabajo): Esta instrucción representa una syscall de I/O bloqueante. Se deberá devolver
// el Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea
// utilizar el proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).
void ejecutar_IO(char* dispositivo, char* tiempo) {
    // serializar_pcb(crear_paquete(PCB_BLOCK), pcb);
    // enviar_paquete(pcb, cliente_servidor_dispatch);  // dispatch
}

// EXIT Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización.
void ejecutar_EXIT(t_pcb* pcb) {
    pthread_mutex_lock(&pedidofin);
    parar_proceso++;
    pthread_mutex_unlock(&pedidofin);
}

int checkInterrupt() {
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
    int cod_op;
    while (1) {
        cod_op = recibir_operacion(conexion_memoria);
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

int indice_registro(char* registro) {
    if (strcmp(registro, "AX") == 0)
        return 0;
    if (strcmp(registro, "BX") == 0)
        return 1;
    if (strcmp(registro, "CX") == 0)
        return 2;
    if (strcmp(registro, "DX") == 0)
        return 3;
}

void copiar_registros_de_pcb(t_pcb* pcb) {
    registros[0] = pcb_recibido->registro->AX;
    registros[1] = pcb_recibido->registro->BX;
    registros[2] = pcb_recibido->registro->CX;
    registros[3] = pcb_recibido->registro->DX;
}