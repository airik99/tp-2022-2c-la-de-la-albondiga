#include <swap.h>

void cargar_pagina(int pid, int id_tabla, int segmento, int num_pagina) {
    pthread_mutex_lock(&mx_tablas_paginas);
    t_list* tabla_paginas = list_get(tablas_paginas, id_tabla);
    pthread_mutex_unlock(&mx_tablas_paginas);

    pthread_mutex_lock(&mx_procesos_cargados);
    proceso_en_memoria* proceso = obtener_proceso_por_pid(pid);
    pthread_mutex_unlock(&mx_procesos_cargados);

    t_pagina* pagina = list_get(tabla_paginas, num_pagina);

    int numero_marco = elegir_marco(proceso, segmento, num_pagina);
    pthread_mutex_lock(&mx_tablas_paginas);
    pagina->marco = numero_marco;
    pagina->uso = 1;
    pagina->presencia = 1;
    pthread_mutex_unlock(&mx_tablas_paginas);

    pthread_mutex_lock(&mx_lista_marcos);
    t_marco* marco = list_get(lista_marcos, numero_marco);
    pthread_mutex_unlock(&mx_lista_marcos);

    marco->pagina = pagina;
    marco->numero_pagina = num_pagina;
    marco->segmento = segmento;
    marco->pid = pid;

    usleep(config_valores.retardo_swap * 1000);
    int pos_swap = pagina->posicion_swap;

    pthread_mutex_lock(&mx_espacio_memoria);
    void* posicion_memoria = espacio_memoria + pagina->marco * config_valores.tam_pagina;
    fseek(fp, pos_swap, SEEK_SET);
    fread(posicion_memoria, config_valores.tam_pagina, 1, fp);
    pthread_mutex_unlock(&mx_espacio_memoria);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "SWAP IN - PID: <%d> - Marco: <%d> - Page In: <%d> | <%d>",
             pid, numero_marco, segmento, num_pagina);
    pthread_mutex_unlock(&mx_log);
}

int elegir_marco(proceso_en_memoria* proceso, int num_segmento, int num_pagina) {
    t_marco* marco;
    int numero_marco;
    if (hay_marcos_disponibles(proceso)) {
        numero_marco = primero_libre(bit_array_marcos_libres, floor(config_valores.tam_memoria / config_valores.tam_pagina));
        list_add(proceso->lista_marcos_asignados, numero_marco);

        pthread_mutex_lock(&mx_lista_marcos);
        marco = list_get(lista_marcos, numero_marco);
        pthread_mutex_unlock(&mx_lista_marcos);

        bitarray_set_bit(bit_array_marcos_libres, numero_marco);
        cantidad_marcos_libres--;
    } else {
        numero_marco = (*algoritmo_reemplazo)(proceso);
        pthread_mutex_lock(&mx_lista_marcos);
        marco = list_get(lista_marcos, numero_marco);
        pthread_mutex_unlock(&mx_lista_marcos);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "REEMPLAZO - PID: <%d> - Marco: <%d> - Page Out: <%d> | <%d> - Page In: <%d> | <%d> ",
                 proceso->pid, numero_marco, marco->segmento, marco->numero_pagina, num_segmento, num_pagina);
        pthread_mutex_unlock(&mx_log);
        descargar_pagina(marco->pagina);
    }
    return numero_marco;
}

int algoritmo_clock(proceso_en_memoria* proceso) {
    int numero_marco;
    t_marco* marco;
    t_pagina* pagina;

    while (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {
        numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
        pthread_mutex_lock(&mx_lista_marcos);
        marco = list_get(lista_marcos, numero_marco);
        pthread_mutex_unlock(&mx_lista_marcos);
        pagina = marco->pagina;
        proceso->indice_ptro_remplazo++;
        if (proceso->indice_ptro_remplazo >= list_size(proceso->lista_marcos_asignados))
            proceso->indice_ptro_remplazo = 0;
        if (pagina->uso == 0)
            return numero_marco;
        pagina->uso = 0;
    }
}

int algoritmo_clock_mejorado(proceso_en_memoria* proceso) {
    int numero_marco;
    t_marco* marco;
    t_pagina* pagina;
    bool selecciono_marco = false;
    while (!selecciono_marco) {
        while (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {
            numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
            pthread_mutex_lock(&mx_lista_marcos);
            marco = list_get(lista_marcos, numero_marco);
            pthread_mutex_unlock(&mx_lista_marcos);
            pagina = marco->pagina;
            proceso->indice_ptro_remplazo++;
            if (pagina->uso == 0 && pagina->modificado == 0)
                return numero_marco;
        }
        if (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {  // encontro 0/0
            selecciono_marco = true;
        } else {
            proceso->indice_ptro_remplazo = 0;  // Dio vuelta completa no encontro 0/0 reinicio el puntero
            while (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {
                numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
                pthread_mutex_lock(&mx_lista_marcos);
                marco = list_get(lista_marcos, numero_marco);
                pthread_mutex_unlock(&mx_lista_marcos);
                pagina = marco->pagina;
                proceso->indice_ptro_remplazo++;
                if (pagina->uso == 0)
                    return numero_marco;
                pagina->uso = 0;
            }
            if (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados))  // encontro 0/1
                selecciono_marco = true;
            else
                proceso->indice_ptro_remplazo = 0;  // no encontro 0/1 reinicio el puntero y se vuelve a ejecutar el algo
        }
    }
    return numero_marco;
}

void descargar_pagina(t_pagina* pagina) {
    pthread_mutex_lock(&mx_lista_marcos);
    t_marco* marco = list_get(lista_marcos, pagina->marco);
    pthread_mutex_unlock(&mx_lista_marcos);
    if (pagina->modificado) {
        int inicio = pagina->marco * config_valores.tam_pagina;
        pthread_mutex_lock(&mx_espacio_memoria);
        void* datos = espacio_memoria + inicio;
        usleep(config_valores.retardo_swap * 1000);
        fseek(fp, pagina->posicion_swap, SEEK_SET);
        fwrite(datos, config_valores.tam_pagina, 1, fp);
        pthread_mutex_unlock(&mx_espacio_memoria);
        pthread_mutex_lock(&mx_log);
        log_info(logger, "SWAP OUT - PID: <%d> - Marco: <%d> - Page Out:<%d>|<%d>", marco->pid, pagina->marco, marco->segmento, marco->numero_pagina);
        pthread_mutex_unlock(&mx_log);
    }
    pagina->presencia = 0;
    pagina->modificado = 0;
    bitarray_clean_bit(bit_array_marcos_libres, pagina->marco);
}

int asginar_espacio_swap() {
    void* vacio = malloc(config_valores.tam_pagina);
    memset(vacio, '0', config_valores.tam_pagina);

    int indice = primero_libre(bit_array_swap, floor((double)config_valores.tam_swap / config_valores.tam_pagina));
    int inicio = indice * config_valores.tam_pagina;
    fseek(fp, inicio, SEEK_SET);
    fwrite(vacio, config_valores.tam_pagina, 1, fp);
    bitarray_set_bit(bit_array_swap, indice);
    free(vacio);
    return inicio;
}
