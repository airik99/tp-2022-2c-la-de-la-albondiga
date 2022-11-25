#include "memoria.h"

config_memoria config_valores;
t_config* config;
t_log* logger;
t_list* tablas_segmentos;
void* espacio_memoria;
pthread_t manejar_conexion_cpu, manejar_conexion_kernel;
int socket_cpu, socket_kernel, socket_servidor;
char* ip;

int main(int argc, char ** argv){
	espacio_memoria = malloc(config_valores.tam_memoria);

    logger = log_create("cfg/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);
	ip = "127.0.0.1";

    cargar_configuracion(); 

	FILE * fp;

	fp = fopen (config_valores.path_swap, "w+");

	//Si no funca el ftruncate, usar el truncate normal
	ftruncate(fileno(fp), config_valores.tam_swap);
	lista_tablas = list_create();
	
	int socket_servidor = iniciar_servidor(config_valores.puerto);

    if (socket_servidor == -1)
    {
        log_info(logger, "Error al iniciar el servidor");
        return EXIT_FAILURE;
    }

    log_info(logger, "Memoria lista para recibir clientes");

    socket_cpu = esperar_cliente(socket_servidor);

    log_info(logger, "CPU conectada. Cerrando programa");

	socket_kernel = esperar_cliente(socket_servidor);

	log_info(logger, "Kernel conectado. Cerrando programa");

	//escuchar_clientes();

	
    pthread_create(&manejar_conexion_cpu, NULL, (void*) escuchar_clientes, (void*) socket_cpu);
    pthread_create(&manejar_conexion_kernel, NULL, (void*) escuchar_clientes, (void*) socket_kernel);

    pthread_join(manejar_conexion_cpu, NULL);
    pthread_join(manejar_conexion_kernel, NULL);

	liberar_conexion(socket_cpu);
	config_destroy(config);
	log_destroy(logger);
	fclose(fp);
	free(espacio_memoria);
	return EXIT_SUCCESS;
}

void cargar_configuracion() {
    
	config = config_create("cfg/Memoria.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

	config_valores.entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	config_valores.marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");
	config_valores.path_swap = config_get_string_value(config, "PATH_SWAP");
	config_valores.puerto = config_get_string_value(config, "PUERTO_ESCUCHA");
	config_valores.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
	config_valores.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
	config_valores.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	config_valores.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    config_valores.tam_swap = config_get_int_value(config, "TAMANIO_SWAP");

    log_info(logger, "Termino de leer el archivo de configuracion");

}

int escuchar_clientes(int socket){
	t_pcb* paquete;
	t_list* lista;
	int cod_op;
		while(1){
			cod_op = recibir_operacion(socket);
			switch (cod_op) {
			case HANDSHAKE:
				log_info(logger, "Recibi un handshake");
				break;
			case INICIAR_PROCESO:
				log_info(logger, "Recibi solicitud de iniciar proceso");
				lista = recibir_segmentos(socket);
				iniciar_tabla(lista);
				//TODO manejar la page fault de planificacion de kernel
				break;
			case PAGE_FAULT_REQUEST:
				log_info(logger, "Recibi un page fault");
				paquete = recibir_pcb(socket);
				//TODO manejar la page fault de planificacion de kernel
				break;
			case ESCRITURA_MEMORIA:
				log_info(logger, "Recibi una escritura");
				paquete = recibir_pcb(socket); //Me llega direccion desde cpu
				//TODO Guardar en memoria la direccion
				break;
			case LECTURA_MEMORIA:
				log_info(logger, "Recibi una lectura");
				paquete = recibir_pcb(socket);
				//TODO
				break;
			case ACCESO_TABLA_PAGINAS:
				log_info(logger, "Recibi un acceso a tabla de paginas");
				paquete = recibir_pcb(socket);
				//TODO
				break;
			case ACCESO_ESPACIO_USUARIO:
				log_info(logger, "Recibi un acceso a espacio de usuario");
				paquete = recibir_pcb(socket);
				//TODO
				break;
			case EXIT:
				log_info(logger, "Salgo del programa");
				paquete = recibir_pcb(socket);
				//TODO
				break;
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

	socket_servidor = iniciar_servidor(config_valores.puerto);

	sleep(5);

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

void iniciar_tabla(t_list* segmentos){
	/*t_list* direcciones = list_create();
	for(int i = 0, i < list_size(segmentos), i++){
		int tamanio_segmento = list_get(segmentos, i);
		entrada_tabla_paginas* entrada = malloc(sizeof(entrada_tabla_paginas));
		entrada->marco = 0;
		entrada->presencia = 0;
		entrada->uso = 0;
		entrada->modificado = 0;
		entrada->posicion_swap = cargar_en_swap(tamanio_segmento); 
		list_add(lista)
	}*/
}

//TODO
int cargar_en_swap(int tamanio_segmento){
	return 0;
}