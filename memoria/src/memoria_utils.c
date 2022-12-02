#include <memoria_utils.h>

config_memoria config_valores;
t_config* config;
t_log* logger;
t_list *tablas_paginas, *procesos_cargados, *lista_marcos;
int cantidad_marcos_libres;
void *espacio_memoria, *marcos_libres, *swap_libre;
FILE* fp;
t_bitarray* bit_array_swap;
t_bitarray* bit_array_marcos_libres;
int (*algoritmo_reemplazo)(proceso_en_memoria*);

pthread_t manejar_conexion_cpu, manejar_conexion_kernel;
int socket_cpu, socket_kernel, socket_servidor;

void cargar_configuracion() {
    config = config_create("cfg/Memoria.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

    config_valores.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
    config_valores.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
    config_valores.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    config_valores.tam_swap = config_get_int_value(config, "TAMANIO_SWAP");

    config_valores.entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
    config_valores.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
    config_valores.algoritmo_reemplazo = config_get_string_value(config, "ALGORITMO_REEMPLAZO");
    config_valores.marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");
    config_valores.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
    config_valores.path_swap = config_get_string_value(config, "PATH_SWAP");

    log_info(logger, "Termino de leer el archivo de configuracion");
}

int primero_libre(t_bitarray* bitarray, int cantidad_elementos) {
    int i;
    for (i = 0; i < cantidad_elementos; i++)
        if (bitarray_test_bit(bitarray, i) == 0)
            return i;
    return -1;
}

int cantidad_marcos_asignados(proceso_en_memoria* p) {
    return list_size(p->lista_marcos_asignados);
}

bool hay_marcos_disponibles(proceso_en_memoria* p) {
    return cantidad_marcos_libres != 0 && (cantidad_marcos_asignados(p) < config_valores.marcos_por_proceso);
}


bool menor_pid(proceso_en_memoria* a, proceso_en_memoria* b) {
    return a->pid < b->pid;
}

proceso_en_memoria* obtener_proceso_por_pid(int pid) {
    bool _es_el_que_busco(proceso_en_memoria* proceso) {
        return proceso->pid == pid;
    }

    proceso_en_memoria* proceso = list_find(procesos_cargados, _es_el_que_busco);
    return proceso;
}

void manejador_seniales(int senial) {
    switch (senial) {
        case SIGINT:
            log_info(logger, "Cerrando hilos");
            pthread_cancel( manejar_conexion_kernel);
            break;
    }
}
