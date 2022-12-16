#include <cpu_utils.h>

config_cpu config_valores;
t_config* config;
t_log* logger;
int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt, tam_pagina, cant_entradas_por_tabla;
pthread_t t_conexion_memoria, hilo_dispatch, hilo_interrupt, pedidofin;
int num_pagina_actual, num_segmento_actual;
int registros[] = {0, 0, 0, 0};
int flag_salida, interrupcion;
t_pcb* pcb_actual;
t_list* tlb;
pthread_mutex_t mx_log, mx_interrupcion;

void cargar_configuracion(char* path) {
    config = config_create(path);

    config_valores.entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    config_valores.reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
    config_valores.retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    config_valores.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
}

void liberar_todo() {
    config_destroy(config);
    log_destroy(logger);
    pthread_mutex_destroy(&mx_interrupcion);
    pthread_mutex_destroy(&mx_log);
    liberar_conexion(conexion_memoria);
    liberar_conexion(socket_servidor_dispatch);
    liberar_conexion(cliente_servidor_dispatch);
    list_destroy_and_destroy_elements(tlb, free);
}

void copiar_valores_registros(int* origen, int* destino) {
    for (int i = 0; i < 4; i++)
        *(destino + i) = *(origen + i);
}
