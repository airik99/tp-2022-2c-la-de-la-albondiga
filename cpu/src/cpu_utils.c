#include <cpu_utils.h>

config_cpu config_valores;
t_config* config;
t_log* logger;
t_handshake* configuracion_tabla;
int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt;
pthread_t conexion_memoria_i, hilo_dispatch, hilo_interrupt, pedidofin;
int ultimo_pid = 0;
int registros[] = {0, 0, 0, 0};
int flag_salida, interrupcion;


void cargar_configuracion() {
    config = config_create("cfg/Cpu.config");
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

void copiar_valores_registros(int* origen, int* destino) {
    for (int i = 0; i < 4; i++)
        *(destino + i) = *(origen + i);
}

void finalizar(){
    pthread_cancel(hilo_dispatch);
    pthread_cancel(hilo_interrupt);
}
