#include "memoria.h"

int main(int argc, char** argv) {
    signal(SIGINT, manejador_seniales);
    espacio_memoria = malloc(config_valores.tam_memoria);
    logger = log_create("cfg/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
    cargar_configuracion();
    pthread_mutex_init(&mx_conexion, NULL);

    if (strcmp(config_valores.algoritmo_reemplazo, "CLOCK") == 0)
        algoritmo_reemplazo = &algoritmo_clock;
    else
        algoritmo_reemplazo = &algoritmo_clock_mejorado;

    espacio_memoria = malloc(config_valores.tam_memoria);
    memset(espacio_memoria, '0', config_valores.tam_memoria);

    cantidad_marcos_libres = ceil(config_valores.tam_memoria / config_valores.tam_pagina);
    lista_marcos = list_create();
    for (int i = 0; i < cantidad_marcos_libres; i++) {
        t_marco* marco = malloc(sizeof(marco));
        list_add(lista_marcos, marco);
    }

    int nro_bytes = ceil((double)cantidad_marcos_libres / 8);
    marcos_libres = malloc(nro_bytes);
    memset(marcos_libres, '0', nro_bytes);
    nro_bytes = ceil((double)config_valores.tam_swap / (config_valores.tam_pagina * 8));
    swap_libre = malloc(nro_bytes);
    memset(swap_libre, '0', nro_bytes);

    bit_array_swap = bitarray_create_with_mode(marcos_libres, nro_bytes, LSB_FIRST);
    bit_array_marcos_libres = bitarray_create_with_mode(swap_libre, nro_bytes, LSB_FIRST);

    procesos_cargados = list_create();

    fp = fopen(config_valores.path_swap, "w+");

    // Si no funca el ftruncate, usar el truncate normal
    ftruncate(fileno(fp), config_valores.tam_swap);
    tablas_paginas = list_create();  // TODO Falta el tipo de dato de lista_tablas

    int socket_servidor = iniciar_servidor(config_valores.puerto);

    if (socket_servidor == -1) {
        log_info(logger, "Error al iniciar el servidor");
        return EXIT_FAILURE;
    }

    log_info(logger, "Memoria lista para recibir clientes");

    socket_cpu = esperar_cliente(socket_servidor);
    log_info(logger, "CPU conectada");
    t_paquete* paquete = crear_paquete(HANDSHAKE);
    agregar_a_paquete(paquete, &config_valores.entradas_por_tabla, sizeof(int));
    agregar_a_paquete(paquete, &config_valores.tam_pagina, sizeof(int));
    enviar_paquete(paquete, socket_cpu);
    eliminar_paquete(paquete);

    socket_kernel = esperar_cliente(socket_servidor);
    log_info(logger, "Kernel conectado");

    pthread_create(&manejar_conexion_kernel, NULL, (void*)escuchar_clientes, (void*)socket_kernel);
    pthread_create(&manejar_conexion_cpu, NULL, (void*)escuchar_clientes, (void*)socket_cpu);
    pthread_join(manejar_conexion_kernel, NULL);
    pthread_join(manejar_conexion_cpu, NULL);

    liberar_conexion(socket_cpu);
    liberar_conexion(socket_kernel);

    config_destroy(config);
    log_destroy(logger);
    fclose(fp);
    free(espacio_memoria);
    return EXIT_SUCCESS;
}

int escuchar_clientes(int socket) {
    t_list* lista;
    t_paquete* paquete;
    int cod_op, id_tabla, pagina, respuesta;
    u_int32_t direccion, valor;
    while (1) {
        // pthread_mutex_lock(&mx_conexion);
        cod_op = recibir_operacion(socket);
        lista = recibir_lista(socket);
        switch (cod_op) {
            case INICIAR_PROCESO:
                log_info(logger, "Recibi solicitud de iniciar proceso");
                t_list* lista_id_tp = iniciar_estructuras(lista);
                paquete = crear_paquete(INICIAR_PROCESO);
                serializar_lista(paquete, lista_id_tp);
                enviar_paquete(paquete, socket);
                break;
            case PAGE_FAULT:
                id_tabla = list_get(lista, 0);
                pagina = list_get(lista, 1);
                cargar_pagina(id_tabla, pagina);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                break;
            case ACCESO_TABLA_PAGINAS:
                id_tabla = list_get(lista, 0);
                pagina = list_get(lista, 1);
                int numero_marco = obtener_marco(id_tabla, pagina);
                send(socket, &numero_marco, sizeof(int), MSG_WAITALL);
                break;
            case LEER_DE_MEMORIA:
                direccion = list_get(lista, 0);
                u_int32_t leido = leer_memoria(direccion);
                send(socket, &leido, sizeof(u_int32_t), MSG_WAITALL);
                break;
            case ESCRIBIR_EN_MEMORIA:
                direccion = list_get(lista, 0);
                valor = list_get(lista, 1);
                escribir_en_memoria(valor, direccion);
                respuesta = 0;
                send(socket, &respuesta, sizeof(int), MSG_WAITALL);
                break;
            case EXIT:
                int pid = list_get(lista, 0);
                finalizar_proceso(pid);
                break;
            case -1:
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                return EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                break;
        }
        free(lista);
        // pthread_mutex_unlock(&mx_conexion);
    }
    return 0;
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

    for (int i = 1; i < list_size(tablas_paginas) && entrada->pid != pid; i++)
        entrada = list_get(tablas_paginas, i);

    while (entrada->pid == pid) {
        list_iterate(entrada->tabla_de_paginas, (void*)liberar_swap_pagina);
        entrada = list_get(tablas_paginas, i);
        i++;
    }

    proceso_en_memoria* proceso = obtener_proceso_por_pid(pid);
    list_iterate(proceso->lista_marcos_asignados, (void*)liberar_marcos_pagina);
    list_destroy(proceso->lista_marcos_asignados);
    free(proceso);
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