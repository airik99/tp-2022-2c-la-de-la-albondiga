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
		log_info(logger, "La pagina %d se encontró en la TLB! \n", num_pagina);
		marco = obtener_marco_tlb(num_pagina);
		direccion_fisica = marco * tam_pagina + desplazamiento_pagina;
		actualizar_ultima_pagina_referenciada(num_pagina);
	} else {
		if(es_direccion_valida(num_pagina, desplazamiento_pagina)) {

			marco = obtener_marco();

			if(1) {

			}

			if(tlb_llena()) {

			} else {
				
			}

		}
	}

}

bool es_direccion_valida(uint32_t num_pagina, uint32_t desplazamiento_pagina) {
	/*t_paquete* paquete = armar_paquete*/
}

void actualizar_ultima_pagina_referenciada(uint32_t num_pagina) {

}

uint32_t obtener_marco_tlb(uint32_t num_pagina) {

}

//////////////////////////////////////// TLB [ pid | segmento | página | marco ]
// Solamente se permiten agregar campos que faciliten la implementación de los algoritmos de reemplazo como "instante de carga" o "instante de última referencia".
//algoritmos: FIFO o LRU
