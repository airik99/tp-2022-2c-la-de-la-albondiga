#include <cpu_utils.h>
#include <mmu.h>

int traducir_direccion_logica(int direccion_logica) {
    int tam_max_segmento = cant_entradas_por_tabla * tam_pagina;
    int num_segmento = floor(direccion_logica / tam_max_segmento);
    int desplazamiento_segmento = direccion_logica % tam_max_segmento;
    int num_pagina = floor(desplazamiento_segmento / tam_pagina);
    int desplazamiento_pagina = desplazamiento_segmento % tam_pagina;

    int respuesta;
    int marco;
    int direccion_fisica;

    num_pagina_actual = num_pagina;
    num_segmento_actual = num_segmento;

    if (!es_direccion_fisica_valida(desplazamiento_segmento, num_segmento)) {
        copiar_valores_registros(registros, (pcb_actual->registro));
        t_paquete* paquete = crear_paquete(SEGMENTATION_FAULT);
        serializar_pcb(paquete, pcb_actual);
        enviar_paquete(paquete, cliente_servidor_dispatch);
        eliminar_paquete(paquete);
        return -1;
    }

    t_traduccion* trad = obtener_entrada_tlb(pcb_actual->pid, num_pagina, num_segmento);
    if (trad != NULL) {             // si la pagina está en la tlb
        marco = acceder_tlb(trad);  // tlb hit
    } else {
        log_info(logger, "PID: <%d> - TLB MISS - Segmento: <%d> - Pagina: <%d>\n", pcb_actual->pid, num_segmento, num_pagina);  // log obligatorio
        respuesta = esta_en_memoria(num_pagina, num_segmento);
        if (respuesta == -1) {
            log_info(logger, "Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>", pcb_actual->pid, num_segmento, num_pagina);  // log obligatorio
            copiar_valores_registros(registros, (pcb_actual->registro));
            t_paquete* paquete = crear_paquete(PAGE_FAULT);
            agregar_a_paquete(paquete, &num_segmento, sizeof(int));
            agregar_a_paquete(paquete, &num_pagina, sizeof(int));
            serializar_pcb(paquete, pcb_actual);
            enviar_paquete(paquete, cliente_servidor_dispatch);
            eliminar_paquete(paquete);
            return -1;
        }
        marco = respuesta;
        if (list_size(tlb) > 0) {
            t_traduccion* traduccion = malloc(sizeof(t_traduccion));
            traduccion->pagina = num_pagina;
            traduccion->marco = marco;
            traduccion->segmento = num_segmento;
            traduccion->pid = pcb_actual->pid;
            traduccion->instante_ultima_referencia = time(NULL);

            bool _mismo_marco(t_traduccion * entrada_tlb) {
                return entrada_tlb->marco = traduccion->marco;
            }

            list_remove_and_destroy_by_condition(tlb, _mismo_marco, free);
            if (tlb_llena()) {
                reemplazo_tlb(traduccion);
            } else {
                agregar_a_tlb(traduccion);
            }
            free(traduccion);
        }
    }
    direccion_fisica = marco * tam_pagina + desplazamiento_pagina;
    return direccion_fisica;
}

int esta_en_memoria(u_int32_t num_pagina, u_int32_t num_segmento) {
    int respuesta;
    t_segmento* seg = list_get(pcb_actual->tabla_segmentos, num_segmento);
    u_int32_t indice = seg->indice_tabla_paginas;
    t_paquete* paquete = crear_paquete(ACCESO_TABLA_PAGINAS);  // TODO: aca hay que acordarnos de evaluar este cod_op en memoria
    agregar_a_paquete(paquete, &indice, sizeof(u_int32_t));
    agregar_a_paquete(paquete, &num_pagina, sizeof(u_int32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
    recv(conexion_memoria, &respuesta, sizeof(int), MSG_WAITALL);
    return respuesta;  // esto nos deberia devolver un marco ó -1
}

bool es_direccion_fisica_valida(u_int32_t desplazamiento_segmento, u_int32_t numero_segmento) {
    t_segmento* segmento = list_get(pcb_actual->tabla_segmentos, numero_segmento);
    int tamio_seg = segmento->tamanio_segmento;
    return desplazamiento_segmento <= tamio_seg - 1;
}
