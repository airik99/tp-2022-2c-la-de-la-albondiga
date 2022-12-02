#include <tlb.h>

void agregar_a_tlb(t_traduccion* traduccion) {
    list_add(tlb, traduccion);
    imprimir_tlb();
}

void reemplazo_tlb(t_traduccion* traduccion) {
    t_traduccion* traduccion_reemplazada;
    if (es_algoritmo("FIFO")) {
        traduccion_reemplazada = list_remove(tlb, 0);
        free(traduccion_reemplazada);
        agregar_a_tlb(traduccion);
    } else if (es_algoritmo("LRU")) {
        traduccion_reemplazada = list_get_minimum(tlb, traduccion_con_referencia_mas_lejana);
        bool es_la_buscada(t_traduccion * t2) {
            return traduccion == t2;
        }
        list_remove_and_destroy_by_condition(tlb, es_la_buscada, free);
        agregar_a_tlb(traduccion);
    }
}

void imprimir_tlb() {
    t_traduccion* t;

    if (tlb == NULL) {
        log_info(logger, "La TLB esta vacia\n");
    } else {
        for (int i = 0; i < list_size(tlb); i++) {
            t = list_get(tlb, i);
            log_info(logger, "<ENTRADA: %d> | PID: <%d> | SEGMENTO: <%d> | PAGINA: <%d> | MARCO: <%d>\n", i, t->pid, t->segmento, t->pagina, t->marco);
        }
    }
}

t_traduccion* traduccion_con_referencia_mas_lejana(t_traduccion* a, t_traduccion* b) {
    return a->instante_ultima_referencia < b->instante_ultima_referencia ? a : b;
}

void inicializar_tlb() {
    tlb = list_create();
}

void vaciar_tlb() {
    list_clean_and_destroy_elements(tlb, free);
    imprimir_tlb();  // log obligatorio
}

bool tlb_llena() {
    return list_size(tlb) == config_valores.entradas_tlb;
}

int acceder_tlb(t_traduccion* traduccion) {
    log_info(logger, "PID: <%d> - TLB HIT - Segmento: <%d> - Pagina: <%d>\n", traduccion->pid, traduccion->segmento, traduccion->pagina);  // log obligatorio
    actualizar_ultima_referencia(traduccion);
    imprimir_tlb();  // log obligatorio
    return traduccion->marco;
}

t_traduccion* obtener_entrada_tlb(uint32_t pid, uint32_t num_pagina, uint32_t num_segmento) {
    bool buscar_traduccion_en_tlb(t_traduccion * traduccion) {
        return traduccion->pid == pid && traduccion->pagina == num_pagina && traduccion->segmento == num_segmento;
    }
    return list_find(tlb, (void*)buscar_traduccion_en_tlb);
}

bool esta_en_tlb(uint32_t pid, uint32_t num_pagina, uint32_t num_segmento) {
    return obtener_entrada_tlb(pid, num_pagina, num_segmento) != NULL;
}

bool es_algoritmo(char* algoritmo) {
    return strcmp(config_valores.reemplazo_tlb, algoritmo) == 0;
}

void actualizar_ultima_referencia(t_traduccion* traduccion) {
    if (es_algoritmo("LRU")) {
        traduccion->instante_ultima_referencia = time(NULL);
    }
}