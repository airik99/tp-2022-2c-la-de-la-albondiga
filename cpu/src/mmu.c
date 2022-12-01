#include <cpu_utils.h>
 
uint32_t traducir_direccion_logica(uint32_t direccion_logica) {
    
	uint32_t tam_max_segmento = cant_entradas_por_tabla * tam_pagina;
	uint32_t num_segmento = floor(direccion_logica /  tam_max_segmento);
	uint32_t desplazamiento_segmento = direccion_logica % tam_max_segmento;
	uint32_t num_pagina = floor(desplazamiento_segmento  / tam_pagina);
	uint32_t desplazamiento_pagina = desplazamiento_segmento %  tam_pagina;

	log_info(logger, "La MMU realizo la siguiente traduccion de la direccion logica\n");
	log_info(logger, "Numero de pagina: %d \n",  num_pagina);
	log_info(logger, "Desplazamiento: %d \n" ,  desplazamiento_pagina);

	uint32_t respuesta;
	uint32_t marco;
	uint32_t direccion_fisica;

	num_pagina_actual = num_pagina;
	num_segmento_actual = num_segmento;

	if(esta_en_tlb(num_pagina, num_segmento)) { //si la pagina está en la tlb
		marco = buscar_en_tlb(num_pagina, num_segmento); //tlb hit
	} else { 
		log_info(logger, "PID: <%d> - TLB MISS - Segmento: <%d> - Pagina: <%d>\n", pcb_actual->pid, num_segmento, num_pagina); //log obligatorio
		if(es_direccion_fisica_valida(desplazamiento_segmento, tam_max_segmento)) { 
		respuesta = esta_en_memoria(num_pagina, num_segmento);
			if(respuesta == -1) {
				log_info(logger, "Page Fault PID: <%d> - Segmento: <%d> - Pagina: <%d>", pcb_actual->pid, num_segmento, num_pagina); //log obligatorio
				t_paquete* paquete = crear_paquete(PAGE_FAULT); //TODO: aca hay que acordarnos de evaluar este cod_op en el kernel
				serializar_pcb(paquete, pcb_actual);
				agregar_a_paquete(paquete, num_pagina, sizeof(uint32_t));
				agregar_a_paquete(paquete, num_segmento, sizeof(uint32_t));
				enviar_paquete(paquete, socket_servidor_dispatch);
				eliminar_paquete(paquete);
				return -1; //devuelve -1 porque hay page fault, sigue del lado del kernel el proceso
			} else { 
				marco = respuesta;
				t_traduccion* traduccion = malloc(sizeof(t_traduccion));
				traduccion->pagina = num_pagina;
				traduccion->marco = marco;
				traduccion->segmento = num_segmento;
				traduccion->pid = pcb_actual->pid;
				traduccion->instante_de_carga = 0;
		
				if(tlb_llena()) {
					reemplazo_tlb(traduccion);
				} else {
					agregar_a_tlb(traduccion);
				}
				free(traduccion);
			} 		
		}
	}
	direccion_fisica = marco * tam_pagina + desplazamiento_pagina;
	return direccion_fisica;
}

int esta_en_memoria(uint32_t num_pagina, uint32_t num_segmento){
	uint32_t respuesta;
	t_paquete* paquete = crear_paquete(ACCESO_TABLA_PAGINAS); //TODO: aca hay que acordarnos de evaluar este cod_op en memoria
	agregar_a_paquete(paquete, num_pagina, sizeof(uint32_t));
	agregar_a_paquete(paquete, num_segmento, sizeof(uint32_t));
	enviar_paquete(paquete, conexion_memoria);
	eliminar_paquete(paquete);
	recv(conexion_memoria, &respuesta, sizeof(uint32_t), MSG_WAITALL);
	return respuesta; //esto nos deberia devolver un marco ó -1
}

bool es_direccion_fisica_valida(uint32_t desplazamiento_segmento, uint32_t tam_max_segmento) {
	if(desplazamiento_segmento > tam_max_segmento) { 
		log_error(logger, "Segmentation Fault (SIGSEGV): El desplazamiento del segmento es mayor al tamaño maximo del segmento \n");
		t_paquete* paquete = crear_paquete(SEGMENTATION_FAULT); //TODO: aca hay que acordarnos de evaluar este cod_op en el kernel
		serializar_pcb(paquete, pcb_actual);
		enviar_paquete(paquete, socket_servidor_dispatch);
		eliminar_paquete(paquete);
		return false;
	} else {
		return true;
	}
}