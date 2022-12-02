#include <cpu_utils.h>

void agregar_a_tlb(t_traduccion* traduccion) {
	int i = 0; 
	t_traduccion* aux = malloc(sizeof(t_traduccion));
	while(list_size(tlb->traducciones) > i) { //corre todos los elementos de la lista una posicion a la derecha y agrega el mas nuevo adelante (posicion 0)
		aux = list_replace(tlb->traducciones, i, traduccion);
		traduccion = aux;
		i++;
	} 
	actualizar_ultima_referencia(traduccion);
	imprimir_entradas_tlb(); //log obligatorio
	free(aux);
	free(traduccion);
}

void reemplazo_tlb(t_traduccion* traduccion) {
	t_traduccion* traduccion_reemplazada = malloc(sizeof(t_traduccion));
	
	if(es_algoritmo("FIFO")) {
		agregar_a_tlb(traduccion);
		traduccion_reemplazada = list_remove(tlb->traducciones, config_valores.entradas_tlb);
		log_info(logger, "Por FIFO se acaba de quitar de la TLB la traduccion:\n");
		log_info(logger, "PID: %d, Segmento: %d, Pagina: %d, Marco: %d\n", traduccion_reemplazada->pid, traduccion_reemplazada->segmento, traduccion_reemplazada->pagina, traduccion_reemplazada->marco);
	} else if(es_algoritmo("LRU")) {
		uint32_t indice = buscar_el_maximo_instante_de_carga(tlb->traducciones);
		traduccion_reemplazada = list_replace(tlb->traducciones, indice, traduccion);//remplazamos el que tiene mayor instante de carga por la traduccion que tenemos
		log_info(logger, "Por LRU se acaba de quitar de la TLB la traduccion:\n");
		log_info(logger, "PID: %d, Segmento: %d, Pagina: %d, Marco: %d\n", traduccion_reemplazada->pid, traduccion_reemplazada->segmento, traduccion_reemplazada->pagina, traduccion_reemplazada->marco);
		imprimir_entradas_tlb(); //log obligatorio
	} else {
		log_error(logger, "No se reconoce el algoritmo de reemplazo de la TLB\n");
	}
	
	free(traduccion);
	free(traduccion_reemplazada);
}

void imprimir_entradas_tlb() {
	log_info(logger, "Entradas de la TLB:\n");
	int i = 0;
	t_traduccion* t = malloc(sizeof(t_traduccion));

	if(tlb->traducciones == NULL) {
		log_info(logger, "La TLB esta vacia\n");
	} else {
		while(list_size(tlb->traducciones) > i) {
			t = list_get(tlb->traducciones, i);
			log_info(logger, "<ENTRADA: %d> | PID: <%d> | SEGMENTO: <%d> | PAGINA: <%d> | MARCO: <%d>\n", i, t->pid, t->segmento, t->pagina, t->marco); //log obligatorio
			i++;
		}
	}
	free(t);
}

uint32_t buscar_el_maximo_instante_de_carga(t_list* traducciones) {
	t_traduccion* traduccion_reemplazo = malloc(sizeof(t_traduccion));
	t_traduccion* aux = malloc(sizeof(t_traduccion));
	uint32_t indice = 0;
	for(int i = 0; list_size(traducciones)-1 > i; i++){
		traduccion_reemplazo = list_get(traducciones, i);
		aux = list_get(traducciones, i+1);
		if(traduccion_reemplazo->instante_de_carga < aux->instante_de_carga) {
			traduccion_reemplazo = list_get(traducciones, i+1);
			indice = i+1;
		}
	}
	free(traduccion_reemplazo);
	free(aux);
}

void inicializar_tlb() {
	tlb = malloc(sizeof(t_tlb*));
	tlb->traducciones = list_create();
	log_info(logger, "Se inicializo la TLB\n"); 
}

void vaciar_tlb() {
	log_info(logger, "Se vaciara la TLB\n");
	imprimir_entradas_tlb(); //log obligatorio
	list_clean_and_destroy_elements(tlb->traducciones, free);
}

bool tlb_llena() {
	log_info(logger, "La TLB esta llena\n");
	imprimir_entradas_tlb(); //log obligatorio
	return list_size(tlb->traducciones) == config_valores.entradas_tlb;
}

uint32_t buscar_en_tlb(uint32_t num_pagina, uint32_t  num_segmento) {
	log_info(logger, "PID: <%d> - TLB HIT - Segmento: <%d> - Pagina: <%d>\n", pcb_actual->pid, num_segmento, num_pagina); //log obligatorio
	t_traduccion* traduccion = malloc(sizeof(t_traduccion*));
	
	bool buscar_traduccion_en_tlb(t_traduccion* trad) {
		if(trad->pagina == num_pagina && trad->segmento == num_segmento) {
			return trad;
		}
	}
	traduccion = list_find(tlb->traducciones, (void*) buscar_traduccion_en_tlb);
	uint32_t marco = traduccion->marco;
	actualizar_ultima_referencia(traduccion);
	imprimir_entradas_tlb(); //log obligatorio
	free(traduccion);
	//free(trad);
	return marco;
}

bool esta_en_tlb(uint32_t num_pagina, uint32_t num_segmento) {
	bool buscar_traduccion_en_tlb(t_traduccion* traduccion) {
		return traduccion->pagina == num_pagina && traduccion->segmento == num_segmento;
	}
	return list_find(tlb->traducciones, (void*) buscar_traduccion_en_tlb) != NULL;
}

bool es_algoritmo(char* algoritmo) {
	return strcmp(config_valores.reemplazo_tlb, algoritmo) == 0;
}

//TODO: ASUMIMOS QUE LA TLB ES POR PROCESOS PERO NO ESTAMOS SEGUROS, SI LA TLB ES GLOBAL, ESTA FUNCION HAY QUE CAMBIARLA (CREO)
//solo se hace en LRU porque en este algoritmo sí importa el momento en el que referenciamos la pagina, en FIFO no importa porque siempre saca la primera que entra
void actualizar_ultima_referencia(t_traduccion* traduccion) {
	if(es_algoritmo("LRU")) {
		t_traduccion* actualizar_instante_de_carga(t_traduccion* trad) {
			if(es_la_traduccion_que_busco(trad, traduccion)) {
				trad->instante_de_carga = 0;
			}
			trad->instante_de_carga++;
			return trad;
		}	
		tlb->traducciones = list_map(tlb->traducciones, actualizar_instante_de_carga);
	}
}

bool es_la_traduccion_que_busco(t_traduccion* traduccion, t_traduccion* traduccion_a_buscar) {
	return traduccion->pagina == traduccion_a_buscar->pagina && traduccion->segmento == traduccion_a_buscar->segmento && traduccion->pid == traduccion_a_buscar->pid && traduccion->marco == traduccion_a_buscar->marco;
}