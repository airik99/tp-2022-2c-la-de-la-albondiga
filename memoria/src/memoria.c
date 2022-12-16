#include "memoria.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        puts("FALTO CONFIG");
        return EXIT_FAILURE;
    }

    char* path_config = argv[1];
    logger = log_create("cfg/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
    cargar_configuracion(path_config);
    iniciar_estructuras_memoria();
    if (strcmp(config_valores.algoritmo_reemplazo, "CLOCK") == 0)
        algoritmo_reemplazo = &algoritmo_clock;
    else
        algoritmo_reemplazo = &algoritmo_clock_mejorado;

    socket_servidor = iniciar_servidor(config_valores.puerto);
    if (socket_servidor == -1) {
        log_info(logger, "Error al iniciar el servidor");
        borrar_todo();
        return EXIT_FAILURE;
    }

    log_info(logger, "Memoria lista para recibir clientes");

    int socket_cliente_1 = esperar_cliente(socket_servidor);
    if (socket_cliente_1 == -1) {
        log_info(logger, "Error de conexion con cliente");
        borrar_todo();
        return EXIT_FAILURE;
    }
    handshake_cliente(socket_cliente_1);

    int socket_cliente_2 = esperar_cliente(socket_servidor);
    if (socket_cliente_2 == -1) {
        log_info(logger, "Error de conexion con cliente");
        borrar_todo();
        return EXIT_FAILURE;
    }
    handshake_cliente(socket_cliente_2);

    pthread_join(manejar_conexion_kernel, NULL);
    pthread_join(manejar_conexion_cpu, NULL);

    liberar_conexion(socket_servidor);
    liberar_conexion(socket_cliente_1);
    liberar_conexion(socket_cliente_2);
    borrar_todo();
    return EXIT_SUCCESS;
}

void escuchar_kernel(int socket) {
    t_list* lista;
    t_paquete* paquete;
    int cod_op, id_tabla, pagina, respuesta;
    u_int32_t direccion, valor;
    pthread_mutex_t conexion;
    pthread_mutex_init(&conexion, NULL);
    while (1) {
        cod_op = recibir_operacion(socket);
        // pthread_mutex_lock(&conexion);
        switch (cod_op) {
            case INICIAR_PROCESO:
                lista = recibir_lista(socket);
                t_list* lista_id_tp = iniciar_estructuras(lista);
                paquete = crear_paquete(INICIAR_PROCESO);
                serializar_lista(paquete, lista_id_tp);
                enviar_paquete(paquete, socket);
                eliminar_paquete(paquete);
                list_destroy(lista);
                list_destroy(lista_id_tp);
                break;
            case PAGE_FAULT:
                lista = recibir_lista(socket);
                id_tabla = list_get(lista, 0);
                pagina = list_get(lista, 1);
                cargar_pagina(id_tabla, pagina);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                list_destroy(lista);
                break;
            case PCB_EXIT:
                lista = recibir_lista(socket);
                int pid = list_get(lista, 0);
                finalizar_proceso(pid);
                list_destroy(lista);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                break;
            case -1:
                pthread_mutex_lock(&mx_log);
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                pthread_mutex_unlock(&mx_log);
                // pthread_mutex_unlock(&conexion);
                return;
            default:
                pthread_mutex_lock(&mx_log);
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                pthread_mutex_unlock(&mx_log);
                break;
        }
        // pthread_mutex_unlock(&conexion);
    }
}

void escuchar_cpu(int socket) {
    t_list* lista;
    t_paquete* paquete;
    int cod_op, id_tabla, pagina, respuesta;
    u_int32_t direccion, valor;
    // pthread_mutex_t conexion;
    // pthread_mutex_init(&conexion, NULL);
    while (1) {
        cod_op = recibir_operacion(socket);
        //     pthread_mutex_lock(&conexion);
        switch (cod_op) {
            case ACCESO_TABLA_PAGINAS:
                lista = recibir_lista(socket);
                id_tabla = list_get(lista, 0);
                pagina = list_get(lista, 1);
                int numero_marco = obtener_marco(id_tabla, pagina);
                send(socket, &numero_marco, sizeof(int), MSG_WAITALL);
                list_destroy(lista);
                break;
            case LEER_DE_MEMORIA:
                lista = recibir_lista(socket);
                direccion = list_get(lista, 0);
                u_int32_t leido = leer_memoria(direccion);
                send(socket, &leido, sizeof(u_int32_t), MSG_WAITALL);
                list_destroy(lista);
                break;
            case ESCRIBIR_EN_MEMORIA:
                lista = recibir_lista(socket);
                direccion = list_get(lista, 0);
                valor = list_get(lista, 1);
                escribir_en_memoria(valor, direccion);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                list_destroy(lista);
                break;
            case -1:
                pthread_mutex_lock(&mx_log);
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                pthread_mutex_unlock(&mx_log);
                //           pthread_mutex_unlock(&conexion);
                return;
            default:
                pthread_mutex_lock(&mx_log);
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                pthread_mutex_unlock(&mx_log);
                break;
        }
        // pthread_mutex_unlock(&conexion);
    }
}

t_list* iniciar_estructuras(t_list* tamanios_segmentos) {
    int pid = list_get(tamanios_segmentos, 0);

    proceso_en_memoria* proceso = malloc(sizeof(proceso_en_memoria));
    proceso->pid = pid;
    proceso->lista_marcos_asignados = list_create();
    proceso->indice_ptro_remplazo = 0;
    pthread_mutex_lock(&mx_procesos_cargados);
    list_add_sorted(procesos_cargados, proceso, menor_pid);
    pthread_mutex_unlock(&mx_procesos_cargados);

    t_list* lista_id_tp = list_create();
    for (int i = 1; i < list_size(tamanios_segmentos); i++) {
        int tamanio = list_get(tamanios_segmentos, i);

        entrada_tablas_paginas* entrada_tp = malloc(sizeof(entrada_tablas_paginas));

        entrada_tp->pid = pid;
        entrada_tp->segmento = i - 1;
        entrada_tp->tabla_de_paginas = list_create();

        int cantidad_paginas = crear_tabla_paginas(entrada_tp->tabla_de_paginas, tamanio);
        pthread_mutex_lock(&mx_tablas_paginas);
        list_add(tablas_paginas, entrada_tp);
        pthread_mutex_unlock(&mx_tablas_paginas);

        list_add(lista_id_tp, list_size(tablas_paginas) - 1);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "Tablas creadas PID: <%d> - Segmento: <%d> -TAMAÑO: <%d> paginas", pid, i - 1, cantidad_paginas);
        pthread_mutex_unlock(&mx_log);
    }
    return lista_id_tp;
}

int crear_tabla_paginas(t_list* tabla_paginas, int tamanio_segmento) {
    int cantidad_paginas = ceil((double)tamanio_segmento / config_valores.tam_pagina);
    for (int i = 0; i < cantidad_paginas; i++) {
        t_pagina* pagina = malloc(sizeof(t_pagina));
        pagina->marco = 0;
        pagina->presencia = 0;
        pagina->uso = 0;
        pagina->modificado = 0;
        pagina->posicion_swap = asginar_espacio_swap();
        list_add(tabla_paginas, pagina);
    }
    return cantidad_paginas;
}

void finalizar_proceso(int pid) {
    int i;
    pthread_mutex_lock(&mx_tablas_paginas);
    entrada_tablas_paginas* entrada = list_get(tablas_paginas, 0);
    pthread_mutex_unlock(&mx_tablas_paginas);

    for (i = 1; i < list_size(tablas_paginas) && entrada->pid != pid; i++)
        entrada = list_get(tablas_paginas, i);

    while (entrada->pid == pid && i < list_size(tablas_paginas)) {
        list_iterate(entrada->tabla_de_paginas, (void*)liberar_swap_pagina);
        entrada = list_get(tablas_paginas, i);
        i++;
    }

    pthread_mutex_lock(&mx_procesos_cargados);
    proceso_en_memoria* proceso = obtener_proceso_por_pid(pid);
    pthread_mutex_unlock(&mx_procesos_cargados);
    list_iterate(proceso->lista_marcos_asignados, (void*)liberar_marcos_pagina);
}

void liberar_swap_pagina(t_pagina* pag) {
    int pos_swap = pag->posicion_swap;
    int bit = floor((double)pos_swap / config_valores.tam_pagina);
    bitarray_clean_bit(bit_array_swap, bit);
}

void liberar_marcos_pagina(int numero_marco) {
    bitarray_clean_bit(bit_array_marcos_libres, numero_marco);
    cantidad_marcos_libres++;
}

int obtener_marco(int id_tabla, int num_pagina) {
    int resultado = -1;
    usleep(config_valores.retardo_memoria * 1000);
    pthread_mutex_lock(&mx_tablas_paginas);
    entrada_tablas_paginas* entrada_tp = list_get(tablas_paginas, id_tabla);

    t_pagina* pagina = list_get(entrada_tp->tabla_de_paginas, num_pagina);
    if (pagina->presencia == 1) {
        pagina->uso = 1;
        pthread_mutex_lock(&mx_log);
        log_info(logger, "ACCESO TABLA PAGINAS PID: <%d> - Página: <%d> - Marco: <%d>", entrada_tp->pid, num_pagina, pagina->marco);
        pthread_mutex_unlock(&mx_log);
        resultado = pagina->marco;
    }
    pthread_mutex_unlock(&mx_tablas_paginas);
    return resultado;
}

uint32_t leer_memoria(int direccion) {
    usleep(config_valores.retardo_memoria * 1000);
    uint32_t leido;
    pthread_mutex_lock(&mx_espacio_memoria);
    void* posicion = direccion + espacio_memoria;
    memcpy(&leido, posicion, sizeof(uint32_t));
    pthread_mutex_unlock(&mx_espacio_memoria);

    int num_marco = floor((double)direccion / config_valores.tam_pagina);
    pthread_mutex_lock(&mx_lista_marcos);
    t_marco* marco = list_get(lista_marcos, num_marco);
    pthread_mutex_unlock(&mx_lista_marcos);

    marco->pagina->modificado = 1;
    marco->pagina->uso = 1;
    pthread_mutex_lock(&mx_log);
    log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección física: <%d>", marco->pid, direccion);
    pthread_mutex_unlock(&mx_log);
    return leido;
}

void escribir_en_memoria(u_int32_t valor, int direccion) {
    usleep(config_valores.retardo_memoria * 1000);
    pthread_mutex_lock(&mx_espacio_memoria);
    void* posicion = espacio_memoria + direccion;
    memcpy(posicion, &valor, sizeof(u_int32_t));
    pthread_mutex_unlock(&mx_espacio_memoria);

    int num_marco = floor((double)direccion / config_valores.tam_pagina);
    pthread_mutex_lock(&mx_lista_marcos);
    t_marco* marco = list_get(lista_marcos, num_marco);
    pthread_mutex_unlock(&mx_lista_marcos);
    marco->pagina->modificado = 1;
    marco->pagina->uso = 1;
    pthread_mutex_lock(&mx_log);
    log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d>", marco->pid, direccion);
    pthread_mutex_unlock(&mx_log);
}