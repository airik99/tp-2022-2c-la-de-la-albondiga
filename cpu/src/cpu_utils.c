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

////////////////////////////////////////// MMU
uint32_t traducir_direccion_logica(uint32_t direccion_logica){
    
	uint32_t tam_max_segmento = cant_entradas_por_tabla * tam_pagina;
	uint32_t num_segmento = floor(direccion_logica /  tam_max_segmento);
	uint32_t desplazamiento_segmento = direccion_logica % tam_max_segmento;
	uint32_t num_pagina = floor(desplazamiento_segmento  / tam_pagina);
	uint32_t desplazamiento_pagina = desplazamiento_segmento %  tam_pagina;

	log_info(logger, "La MMU realizo la siguiente traduccion de la direccion logica\n");
	log_info(logger, "Numero de pagina: %d \n",  num_pagina);
	log_info(logger, "Desplazamiento: %d \n" ,  desplazamiento_pagina);

	uint32_t marco;
	uint32_t direccion_fisica;

	if(pagina_esta_en_tlb(num_pagina)) {
		marco = obtener_marco_tlb(num_pagina);
		direccion_fisica = marco * tam_pagina + desplazamiento_pagina;

	}

}
/*uint32_t obtener_direccion_fisica(uint32_t direccion_logica) {
	log_trace(log_test, "Se va a realizar una traduccion de DL a DF del proceso: %i  \n",pcb_global.id);

	uint32_t numero_pagina = (direccion_logica / TAM_PAGINA);// - (direccion_logica % TAM_PAGINA);
	uint32_t tabla_nivel_1 = pcb_global.tabla_paginas;
	uint32_t entrada_tabla_1er_nivel = (numero_pagina / ENTRADAS_POR_TABLA_MEMORIA);
	uint32_t entrada_tabla_2do_nivel =  numero_pagina % (ENTRADAS_POR_TABLA_MEMORIA);
	uint32_t desplazamiento = direccion_logica - (numero_pagina * TAM_PAGINA);
	//printf("El numero de pagina es: %i \n", numero_pagina);
	//printf("El offset es: %i \n", desplazamiento);
	uint32_t direccion_fisica = 0;
	uint32_t marco;

	if(pagina_esta_en_tlb(numero_pagina)) {
		marco = obtener_frame_de_tlb(numero_pagina);
		printf("El frame obtenido de la TLB es: %i \n", marco);
		direccion_fisica = marco * TAM_PAGINA + desplazamiento;
		actualizar_cola_de_algoritmo_por_referencia_a_una_pagina_en_tlb(numero_pagina);
		log_warning(log_test, "La pagina referenciada %i fue encontrada en la tlb  \n", numero_pagina);
	}
	else {
		uint32_t* hubo_reemplazo_en_memoria = malloc(sizeof(uint32_t));
		uint32_t* pagina_reemplazada_en_memoria = malloc(sizeof(uint32_t));
		uint32_t pagina_victima;
		//Acceder a memoria para obtener frame

		int socket_memoria = conectar_con_memoria(IP_MEMORIA,PUERTO_MEMORIA);

		resultado_validacion = validar_direccion_fisica(socket_memoria, pcb_global.id, numero_pagina,desplazamiento);

		if(resultado_validacion) {

			marco = obtener_frame_de_tabla_de_nivel_2(socket_memoria, tabla_nivel_1, entrada_tabla_1er_nivel, entrada_tabla_2do_nivel, hubo_reemplazo_en_memoria, pagina_reemplazada_en_memoria);
			//marco = 5;// TODO: borrar cuando funcione la conexion con memoria

			//Si hay posibles victimas, elegir una
			log_error(log_test," \n EL REMPLAZO ES  %i \n",*hubo_reemplazo_en_memoria);
			if(*hubo_reemplazo_en_memoria) {
				pagina_victima = *pagina_reemplazada_en_memoria;
				quitar_entrada_tlb(pagina_victima);
				actualizar_cola_de_algoritmo_para_dejar_tlb_consistente_por_reemplazo_hecho_en_memoria(pagina_victima, numero_pagina);
				log_trace(log_test, "Hubo reempazo de pagina en memoria del proceso%i . Se reemplazo la pagina %i por la pagina %i en la tlb \n",pcb_global.id, pagina_victima, numero_pagina);
			}

			if(tlb_llena()) {

				pagina_victima = elegir_victima_segun_algoritmo();
				//printf("Se elige la pagina %i como victima \n", pagina_victima);
				quitar_entrada_tlb(pagina_victima);
				//printf("Se quito la entrada de la pagina %i. Ahora hay %i entradas \n", pagina_victima, dictionary_size(tlb));
				actualizar_cola_de_algoritmo_por_reemplazo_hecho(pagina_victima, numero_pagina);
				log_trace(log_test, "No hubo reemplazo de pagina en memoria del proceso: %i . Se reemplazo la pagina %i por la pagina %i en la tlb \n", pcb_global.id,pagina_victima, numero_pagina);

			}else {

				if(!(*hubo_reemplazo_en_memoria))
					actualizar_cola_de_algoritmo_cuando_la_tlb_no_esta_llena(numero_pagina);

			}

			//Guardar página y marco en TLB
			//printf("Se agrega entrada a tlb \n");
			agregar_entrada_a_tlb(numero_pagina, marco);

			//Obtener dirección física
			direccion_fisica = marco * TAM_PAGINA + desplazamiento;
		}

		free(hubo_reemplazo_en_memoria);
		free(pagina_reemplazada_en_memoria);
		liberar_conexion(socket_memoria);
	}


	return direccion_fisica;
}
*/