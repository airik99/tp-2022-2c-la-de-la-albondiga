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
    while (1) {
        cod_op = recibir_operacion(socket);
        pthread_mutex_lock(&mx_conexion);
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
            case EXIT:
                lista = recibir_lista(socket);
                int pid = list_get(lista, 0);
                finalizar_proceso(pid);
                list_destroy(lista);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                break;
            case -1:
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                pthread_mutex_unlock(&mx_conexion);
                return;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
        pthread_mutex_unlock(&mx_conexion);
    }
}

void escuchar_cpu(int socket) {
    t_list* lista;
    t_paquete* paquete;
    int cod_op, id_tabla, pagina, respuesta;
    u_int32_t direccion, valor;
    while (1) {
        cod_op = recibir_operacion(socket);
        pthread_mutex_lock(&mx_conexion);
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
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                pthread_mutex_unlock(&mx_conexion);
                return;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
        pthread_mutex_unlock(&mx_conexion);
    }
}

t_list* iniciar_estructuras(t_list* tamanios_segmentos) {
    int pid = list_get(tamanios_segmentos, 0);

    proceso_en_memoria* proceso = malloc(sizeof(proceso_en_memoria));
    proceso->pid = pid;
    proceso->lista_marcos_asignados = list_create();
    proceso->indice_ptro_remplazo = 0;
    list_add_sorted(procesos_cargados, proceso, menor_pid);

    t_list* lista_id_tp = list_create();
    for (int i = 1; i < list_size(tamanios_segmentos); i++) {
        int tamanio = list_get(tamanios_segmentos, i);

        entrada_tablas_paginas* entrada_tp = malloc(sizeof(entrada_tablas_paginas));

        entrada_tp->pid = pid;
        entrada_tp->segmento = i - 1;
        entrada_tp->tabla_de_paginas = list_create();

        int cantidad_paginas = crear_tabla_paginas(entrada_tp->tabla_de_paginas, tamanio);
        list_add(tablas_paginas, entrada_tp);
        list_add(lista_id_tp, list_size(tablas_paginas) - 1);
        log_info(logger, "Tablas creadas PID: <%d> - Segmento: <%d> -TAMAÑO: <%d> paginas", pid, i - 1, cantidad_paginas);
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
    entrada_tablas_paginas* entrada = list_get(tablas_paginas, 0);

    for (i = 1; i < list_size(tablas_paginas) && entrada->pid != pid; i++)
        entrada = list_get(tablas_paginas, i);

    while (entrada->pid == pid && i < list_size(tablas_paginas)) {
        list_iterate(entrada->tabla_de_paginas, (void*)liberar_swap_pagina);
        entrada = list_get(tablas_paginas, i);
        i++;
    }

    proceso_en_memoria* proceso = obtener_proceso_por_pid(pid);
    list_iterate(proceso->lista_marcos_asignados, (void*)liberar_marcos_pagina);
    eliminar_proceso_en_memoria(proceso);
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
    usleep(config_valores.retardo_memoria * 1000);
    entrada_tablas_paginas* entrada_tp = list_get(tablas_paginas, id_tabla);
    t_pagina* pagina = list_get(entrada_tp->tabla_de_paginas, num_pagina);
    if (pagina->presencia == 1) {
        pagina->uso = 1;
        log_info(logger, "ACCESO TABLA PAGINAS PID: <%d> - Página: <%d> - Marco: <%d>", entrada_tp->pid, num_pagina, pagina->marco);
        return pagina->marco;
    }
    return -1;
}

uint32_t leer_memoria(int direccion) {
    uint32_t leido;
    void* posicion = direccion + espacio_memoria;
    int num_marco = floor((double)direccion / config_valores.tam_pagina);
    t_marco* marco = list_get(lista_marcos, num_marco);
    memcpy(&leido, posicion, sizeof(uint32_t));
    marco->pagina->modificado = 1;
    marco->pagina->uso = 1;

    log_info(logger, "PID: <%d> - Acción: <LEER> - Dirección física: <%d>", marco->pid, direccion);
    return leido;
}

void escribir_en_memoria(u_int32_t valor, int direccion) {
    void* posicion = espacio_memoria + direccion;
    memcpy(posicion, &valor, sizeof(u_int32_t));

    int num_marco = floor((double)direccion / config_valores.tam_pagina);
    t_marco* marco = list_get(lista_marcos, num_marco);

    marco->pagina->modificado = 1;
    marco->pagina->uso = 1;

    log_info(logger, "PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d>", marco->pid, direccion);
}