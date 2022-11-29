#include <cpu_utils.h>

config_cpu config_valores;
t_config* config;
t_log* logger;
t_handshake* configuracion_tabla;
int conexion_memoria, parar_proceso, cliente_servidor_interrupt, cliente_servidor_dispatch, socket_servidor_dispatch, socket_servidor_interrupt, tam_pagina, cant_entradas_por_tabla;
pthread_t conexion_memoria_i, hilo_dispatch, hilo_interrupt, pedidofin;
int ultimo_pid = 0;
int registros[] = {0, 0, 0, 0};
int flag_salida, interrupcion;
t_pcb* pcb_actual;
time_t tiempo;

void cargar_configuracion() {
    config = config_create("cfg/Cpu.config");
    log_info(logger, "Arranco a leer el archivo de configuracion \n");

    config_valores.entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    config_valores.reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
    config_valores.retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    config_valores.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
	
    log_info(logger, "Termino de leer el archivo de configuracion \n");
}

void liberar_todo() {
    liberar_conexion(conexion_memoria);
    liberar_conexion(socket_servidor_dispatch);
    liberar_conexion(cliente_servidor_dispatch);
    config_destroy(config);
    log_destroy(logger);
}

void copiar_valores_registros(int* origen, int* destino) {
    for (int i = 0; i < 4; i++)
        *(destino + i) = *(origen + i);
}

void finalizar(){
    pthread_cancel(hilo_dispatch);
    pthread_cancel(hilo_interrupt);
}

/////////////////////////////////////////////////////////////////  MMU /////////////////////////////////////////////////////////////////  
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

	if(esta_en_tlb(num_pagina, num_segmento)) { //si la pagina está en la tlb
		marco = buscar_en_tlb(num_pagina, num_segmento);
	} else { //si la pagina no está en la tlb, tlb miss
		if(es_direccion_fisica_valida(desplazamiento_segmento, tam_max_segmento)) { 
		respuesta = esta_en_memoria(num_pagina, num_segmento);
			if(respuesta == PAGE_FAULT) { //TODO: aca hay que acordarnos de evaluar este cod_op en memoria
				t_paquete* paquete = crear_paquete(PAGE_FAULT); //TODO: aca hay que acordarnos de evaluar este cod_op en el kernel
				agregar_a_paquete(paquete, pcb_actual, sizeof(t_pcb));
				//agregar todo al pcb (segmento y num pag)
				//TODO: NOS QUEDAMOS POR ACA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				enviar_paquete(paquete, socket_servidor_dispatch);
			} else { 
				marco = respuesta;
				t_traduccion* traduccion = malloc(sizeof(t_traduccion));
				traduccion->pagina = num_pagina;
				traduccion->marco = marco;
				traduccion->segmento = num_segmento;
				traduccion->pid = pcb_actual->pid;
		
				if(tlb_llena()) { //TODO: ver como hacer si la tlb esta llena o no
					agregar_a_tlb(traduccion);
				} else {
					agregar_a_tlb(traduccion);
				}
			} 		
		}
	}
	direccion_fisica = marco * tam_pagina + desplazamiento_pagina;
	return direccion_fisica;
}

/*Número de marco: En este caso finalizamos la traducción, actualizamos la TLB y continuamos con la ejecución.
Page Fault: En este caso deberemos devolver el contexto de ejecución al Kernel sin actualizar el valor del program counter. 
Deberemos indicarle al Kernel qué segmento y número de página fueron los que generaron el page fault para que éste resuelva el mismo.*/
int esta_en_memoria(uint32_t num_pagina, uint32_t num_segmento){
	t_paquete* paquete = crear_paquete(ESTA_EN_MEMORIA); //TODO: aca hay que acordarnos de evaluar este cod_op en memoria
	agregar_a_paquete(paquete, num_pagina, sizeof(uint32_t));
	agregar_a_paquete(paquete, num_segmento, sizeof(uint32_t));
	enviar_paquete(paquete, socket_servidor_dispatch);
	return recibir_respuesta(socket_servidor_dispatch); //esto nos deberia devolver un cod_op (que puede ser: ESTA_EN_MEMORIA ó PAGE_FAULT)
}

//obtenemos el marco de la pagina cargada que ya esta en la RAM
uint32_t obtener_marco(uint32_t direccion_fisica) {
	//accede a memoria y devuelve el marco
	return 1;
}

bool es_direccion_fisica_valida(uint32_t desplazamiento_segmento, uint32_t tam_max_segmento) {
	if(desplazamiento_segmento > tam_max_segmento) { 
		log_error(logger, "Segmentation Fault (SIGSEGV): El desplazamiento del segmento es mayor al tamaño maximo del segmento \n");
		t_paquete* paquete = crear_paquete(SEGMENTATION_FAULT); //TODO: aca hay que acordarnos de evaluar este cod_op en el kernel
		agregar_a_paquete(paquete, pcb_actual, sizeof(t_pcb));
		enviar_paquete(paquete, socket_servidor_dispatch);
		//TODO: aca tenemos que enviar el contexto de ejecucion para que el kernel lo finalice, revisar si se envia así el pcb
		return true;
	} else {
		return false;
	}
}

/////////////////////////////////////////////////////////////////  TLB  ///////////////////////////////////////////////////////////////// 
void agregar_a_tlb(t_traduccion* traduccion) {
	int i = 0;
	t_traduccion* aux = malloc(sizeof(t_traduccion));
	while(list_size(tlb->traducciones) > i) { //corre todos los elementos de la lista una posicion a la derecha y agrega el mas nuevo adelante (posicion 0)
		aux = list_replace(tlb->traducciones, i, traduccion);
		traduccion = aux;
		i++;
	} 
	free(aux);
	free(traduccion);
}

void inicializar_tlb() {
	tlb = malloc(sizeof(t_tlb*));
	tlb->traducciones = list_create();
}

bool tlb_llena() {
	return list_size(tlb->traducciones) == config_valores.entradas_tlb;
}

//busca una pagina en la tlb y devuelve el marco
/*uint32_t obtener_marco_tlb(uint32_t num_pagina, u_int32_t num_segmento) {
	t_traduccion* tradu = malloc(sizeof(t_traduccion*));
	
	bool _buscar_pagina(t_traduccion* traduccion) {
		return traduccion->num_pagina == num_pagina && traduccion->num_segmento == num_segmento;
	}

	tradu = list_find(tlb->traducciones, (void*)_buscar_pagina);
	return tradu->marco;
}*/

//TLB HIT: se encuentra la pagina en la tlb
uint32_t buscar_en_tlb(uint32_t num_pagina, uint32_t  num_segmento) {
	log_info(logger, "La pagina %d del segmento %d se encontró en la TLB! \n", num_pagina, num_segmento);
	t_traduccion* traduccion = malloc(sizeof(t_traduccion*));
	
	bool buscar_traduccion_en_tlb(t_traduccion* trad) {
		if(trad->pagina == num_pagina && trad->segmento == num_segmento) {
			return trad;
		}
	}
	traduccion = list_find(tlb->traducciones, (void*) buscar_traduccion_en_tlb); //TODO: ver como pasar los paremetros para sacar la funcion afuera
	actualizar_ultima_referencia(traduccion);
	return traduccion->marco;
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