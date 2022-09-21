#include "memoria.h"

config_memoria config_valores;
t_config* config;
t_log* logger;
int socket_cpu, socket_kernel, socket_servidor;
char* ip;

int main(int argc, char ** argv){

    logger = log_create("cfg/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
	ip = "127.0.0.1";

    //PRUEBAS
    cargar_configuracion(); 

	conectar_con_clientes();
	//escuchar_clientes();

	return EXIT_SUCCESS;

}

void cargar_configuracion() {
    
	config = config_create("cfg/archivo_configuracion.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

	config_valores.entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	config_valores.marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");
	config_valores.path_swap = config_get_string_value(config, "PATH_SWAP");
	config_valores.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	config_valores.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
	config_valores.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
	config_valores.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	config_valores.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    config_valores.tam_pagina = config_get_int_value(config, "TAMANIO_SWAP");

    log_info(logger, "Termino de leer el archivo de configuracion");

}

int escuchar_clientes(){

	t_list* paquete;
		while(1){
			int cod_op = recibir_operacion(socket_servidor);
			switch (cod_op) {
			case MENSAJE:
				recibir_mensaje(socket_servidor);
				log_info(logger, "Me llego este mensaje\n");
				break;
			/*case PAQUETE:
				paquete = recibir_paquete(socket_servidor);
				log_info(logger, "Me llegaron los siguientes valores:\n");
				list_iterate(paquete, (void*) iterator);
				//atender_paquete(paquete);
				break;*/
			case -1:
				log_error(logger, "El cliente se desconecto. Terminando servidor");
				return EXIT_FAILURE;
			default:
				log_warning(logger,"Operacion desconocida. No quieras meter la pata");
				break;
		}
	}
		return 0;
}

void conectar_con_clientes() {
	log_info(logger, "Iniciando servidor...");

	socket_servidor = iniciar_servidor(ip, config_valores.puerto);

	log_info(logger, "Memoria lista para recibir un cliente\n");

	socket_cpu = esperar_cliente(socket_servidor);

}

void iterator(char* value) {
	log_info(logger,"%s", value);
}

void recibir_mensaje(int socket_cliente) {
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	free(buffer);
}