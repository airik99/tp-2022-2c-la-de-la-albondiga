#include <swap.h>

void cargar_pagina(int id_tabla, int num_pagina) {
    entrada_tablas_paginas* entrada_tp = list_get(tablas_paginas, id_tabla);
    t_pagina* pagina = list_get(entrada_tp->tabla_de_paginas, num_pagina);
    proceso_en_memoria* proceso = obtener_proceso_por_pid(entrada_tp->pid);

    int numero_marco = elegir_marco(proceso, entrada_tp->segmento, num_pagina);

    pagina->marco = numero_marco;
    pagina->uso = 1;
    pagina->presencia = 1;

    t_marco* marco = list_get(lista_marcos, numero_marco);
    marco->pagina = pagina;
    marco->numero_pagina = num_pagina;
    marco->segmento = entrada_tp->segmento;
    marco->pid = entrada_tp->pid;

    int pos_swap = pagina->posicion_swap;
    void* posicion_memoria = espacio_memoria + pagina->marco * config_valores.tam_pagina;
    usleep(config_valores.retardo_swap * 1000);
    fseek(fp, pos_swap, SEEK_SET);
    fread(posicion_memoria, config_valores.tam_pagina, 1, fp);
    log_info(logger, "SWAP IN - PID : <%d> - Marco : <%d> - Page In : <%d> | <%d>",
             proceso->pid, numero_marco, marco->segmento, num_pagina);
}

int elegir_marco(proceso_en_memoria* proceso, int num_segmento, int num_pagina) {
    t_marco* marco;
    int numero_marco;
    if (hay_marcos_disponibles(proceso)) {
        numero_marco = primero_libre(bit_array_marcos_libres, ceil(config_valores.tam_memoria / config_valores.tam_pagina));
        list_add(proceso->lista_marcos_asignados, numero_marco);
        marco = list_get(lista_marcos, numero_marco);
        bitarray_set_bit(bit_array_marcos_libres, numero_marco);
        cantidad_marcos_libres--;
    } else {
        numero_marco = (*algoritmo_reemplazo)(proceso);
        marco = list_get(lista_marcos, numero_marco);
        log_info(logger, "REEMPLAZO - PID : <%d> - Marco : <%d> - Page Out : <%d> | <%d> - Page In : <%d> | <%d> ",
                 proceso->pid, numero_marco, marco->segmento, marco->numero_pagina, num_segmento, num_pagina);
        descargar_pagina(marco->pagina);
    }
    return numero_marco;
}

int algoritmo_clock(proceso_en_memoria* proceso) {
    int numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
    t_marco* marco = list_get(lista_marcos, numero_marco);
    t_pagina* pagina = pagina = marco->pagina;

    while (pagina->uso != 0) {
        pagina->uso = 0;
        proceso->indice_ptro_remplazo++;
        if (proceso->indice_ptro_remplazo >= list_size(proceso->lista_marcos_asignados))
            proceso->indice_ptro_remplazo = 0;
        int numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
        marco = list_get(lista_marcos, numero_marco);
        pagina = marco->pagina;
    }
    return numero_marco;
}

int algoritmo_clock_mejorado(proceso_en_memoria* proceso) {
    int numero_marco;
    t_marco* marco;
    t_pagina* pagina;
    bool selecciono_marco = false;
    while (!selecciono_marco) {
        while (pagina->uso != 0 && pagina->modificado != 0 && proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {
            numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
            marco = list_get(lista_marcos, numero_marco);
            pagina = marco->pagina;
            proceso->indice_ptro_remplazo++;
        }
        if (proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados))  // encontro 0/0
            selecciono_marco = true;
        else {
            proceso->indice_ptro_remplazo = 0;  // Dio vuelta completa no encontro 0/0 reinicio el puntero
            while (pagina->uso != 0 && proceso->indice_ptro_remplazo < list_size(proceso->lista_marcos_asignados)) {
                numero_marco = list_get(proceso->lista_marcos_asignados, proceso->indice_ptro_remplazo);
                marco = list_get(lista_marcos, numero_marco);
                pagina = marco->pagina;
                pagina->uso = 0;
                proceso->indice_ptro_remplazo++;
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
    t_marco* marco = list_get(lista_marcos, pagina->marco);
    if (pagina->modificado) {
        int inicio = pagina->marco * config_valores.tam_pagina;
        void* datos = espacio_memoria + inicio;
        usleep(config_valores.retardo_swap * 1000);
        fseek(fp, pagina->posicion_swap, SEEK_SET);
        fwrite(datos, config_valores.tam_pagina, 1, fp);
        log_info(logger, "SWAP OUT - PID: <%d> - Marco: <%d> - Page Out:<%d>|<%d>", marco->pid, pagina->marco, marco->segmento, marco->numero_pagina);
    }
    pagina->presencia = 0;
    pagina->modificado = 0;
    bitarray_clean_bit(bit_array_marcos_libres, pagina->marco);
}

int asginar_espacio_swap() {
    void* vacio = malloc(config_valores.tam_pagina);
    memset(vacio, '0', config_valores.tam_pagina);

    int indice = primero_libre(bit_array_swap, ceil((double)config_valores.tam_swap / config_valores.tam_pagina));
    int inicio = indice * config_valores.tam_pagina;
    fseek(fp, inicio, SEEK_SET);
    fwrite(vacio, config_valores.tam_pagina, 1, fp);
    bitarray_set_bit(bit_array_swap, indice);

    return inicio;
}
