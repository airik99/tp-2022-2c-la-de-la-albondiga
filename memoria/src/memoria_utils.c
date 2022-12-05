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
pthread_mutex_t mx_conexion;

pthread_t manejar_conexion_cpu, manejar_conexion_kernel;
int socket_cpu, socket_kernel, socket_servidor;

void cargar_configuracion(char* path) {
    config = config_create(path);
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
    bool _es_el_que_busco(proceso_en_memoria * proceso) {
        return proceso->pid == pid;
    }

    proceso_en_memoria* proceso = list_find(procesos_cargados, _es_el_que_busco);
    return proceso;
}

void manejador_seniales(int senial) {
    switch (senial) {
        case SIGINT:
            pthread_mutex_unlock(&mx_conexion);
            pthread_mutex_destroy(&mx_conexion);
            pthread_cancel(manejar_conexion_kernel);
            pthread_cancel(manejar_conexion_cpu);
            break;
    }
}

void borrar_todo() {
    free(espacio_memoria);
    fclose(fp);
    log_destroy(logger);
    config_destroy(config);
    pthread_mutex_destroy(&mx_conexion);
    free(marcos_libres);
    free(swap_libre);
    bitarray_destroy(bit_array_marcos_libres);
    bitarray_destroy(bit_array_swap);
    list_destroy_and_destroy_elements(lista_marcos, free);
    list_destroy_and_destroy_elements(tablas_paginas, eliminar_entrada_tabla_pagina);
    list_destroy(procesos_cargados);
}

void eliminar_proceso_en_memoria(proceso_en_memoria* p) {
    list_destroy(p->lista_marcos_asignados);
    free(p);
}

void eliminar_entrada_tabla_pagina(entrada_tablas_paginas* tp) {
    list_destroy_and_destroy_elements(tp->tabla_de_paginas, free);
    free(tp);
}
