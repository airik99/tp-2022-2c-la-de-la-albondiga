 #include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;
char* ip;
t_pcb* pcb_recibido;
int conexion_memoria, parar_proceso, cliente_servidor_dispatch, socket_servidor_dispatch;
pthread_t conexion_memoria_i, hilo_interrupciones;

int main(int argc, char ** argv){
	
	logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
	pcb_recibido = malloc(sizeof(t_pcb));
    ip = "127.0.0.1"; //esto no debería estar hardcodeado

    cargar_configuracion(); 

	//CONEXION CON MEMORIA
	log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria \n");

	conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);
	error_conexion(conexion_memoria);

	log_info(logger, "Conexion con memoria exitosa \n");

	pthread_create(&conexion_memoria_i, NULL, conexion_inicial_memoria, NULL);

	log_info(logger, "Hilo de conexion con memoria creado \n");

	//INTERRUPCIONES
	log_info(logger,"Iniciando conexion con kernel por interrupt\n");

	pthread_create(&hilo_interrupciones, NULL, conexion_interrupciones, (ip, config_valores.puerto_escucha_interrupt));

	log_info(logger, "Hilo de interrupciones creado \n");

    //CONEXION CON KERNEL
	log_info(logger,"Iniciando conexion con kernel por dispatch\n");

    socket_servidor_dispatch = iniciar_servidor(ip, config_valores.puerto_escucha_dispatch);
    error_conexion(socket_servidor_dispatch);

	cliente_servidor_dispatch = esperar_cliente(socket_servidor_dispatch);

    log_info(logger,"Se conecto un cliente por dispatch\n");

	recibir_codigo_operacion_por_dispatch(cliente_servidor_dispatch);
	
	liberar_todo();
	return EXIT_SUCCESS;
}

void error_conexion(int socket) {
	if (socket == -1) {
		log_info(logger, "Error en la conexion al servidor.\n");
		exit(EXIT_FAILURE);
	}
}

void recibir_codigo_operacion_por_dispatch(int cliente) {
	while (1) { 
        int cod_op = recibir_operacion(cliente);
        evaluar_cod_op(cod_op);
    }
    return EXIT_SUCCESS;
}

void evaluar_cod_op(int cod_op) {
	switch (cod_op) {
		case PCB:
			//parar_proceso = 0 ;//INICIA EL CONTADOR DE PARAR PROCESO
			//pcb_recibido = recibir_pcb(cliente); //aca recibo el pcb pero espero a la funcion de maxi xd
			log_info(logger, "Recibi PCB de Id: %d \n", pcb_recibido->pid);
			/*if(ultimo_pid != pcb_recibido->pid && list_size(tlb->lista) != 0){
				//vaciarTlb();
				log_info(logger,"Se vacio TLB\n");
			}*/
			//ciclo_de_instruccion(pcb_recibido,cliente); //INICIA EL CICLO DE INSTRUCCION
			break;
		case HANDSHAKE:
			log_info(logger, "Se recibio la configuracion por handshake \n");
			t_handshake* configuracion_tabla = recibir_handshake(conexion_memoria);
			return NULL;
			break;
		case INTERRUPCION:
			log_info(logger,"Peticion de interrupcion recibida \n");
			//parar_proceso++;
			break;
		casos_de_error(cod_op);
	}
	return EXIT_SUCCESS;
}

void* conexion_inicial_memoria(){

	pedir_handshake(conexion_memoria);
	log_info(logger, "Se envio el pedido de handshake\n");
	
	while(1){
		int cod_op = recibir_operacion(conexion_memoria);
		evaluar_cod_op(cod_op);
	}
	return NULL;
}

void pedir_handshake(int socket_memoria){
	op_code codigo = HANDSHAKE;
	enviar_datos(socket_memoria, &codigo, sizeof(op_code));
}


t_handshake* recibir_handshake(int socket_memoria) {
	t_handshake* han = malloc(sizeof(t_handshake));
	t_list* valores; 
	//valores = recibir_paquete(socket_memoria); //TODO LA FUNCION DE RECIBIR PAQUETE NO ESTA HECHA
	han->tam_pagina = list_get(valores, 0);
	han->entradas = list_get(valores, 1);

	list_destroy(valores);

	return han;
}

void* conexion_interrupciones(char* ip, char* puerto) {
	int server_fd = iniciar_servidor(ip, puerto);
	log_info(logger, "CPU listo para recibir interrupciones \n");
	int cliente_fd = esperar_cliente(server_fd);
	log_info(logger,"Se conecto Kernel al puerto interrupt \n");
	
	while(1) {
		int cod_op;
		recibir_datos(cliente_fd, &cod_op, sizeof(int));
		evaluar_cod_op(cod_op);
	}
	return NULL;
}

void casos_de_error(int codigo_error){
	switch(codigo_error){
		case -1:
			log_error(logger, "Fallo la comunicacion. Abortando \n");
			return EXIT_FAILURE;
		break;
		default:
			log_warning(logger, "Operacion desconocida \n");
		break;
	}
}

void cargar_configuracion() {
    
	config = config_create("cfg/archivo_configuracion.config");
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
