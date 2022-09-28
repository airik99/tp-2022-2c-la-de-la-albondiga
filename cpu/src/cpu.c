#include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;
char* ip;
int socket_kernel;

int main(int argc, char ** argv){
	
	logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);

    ip = "127.0.0.1";

    cargar_configuracion(); 

    //CONEXION CON MEMORIA
	log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria");

	int conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);

	if (conexion_memoria == -1) {
		log_info(logger, "Error en la conexion al servidor. Terminando consola");
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion con memoria exitosa");

    //CONEXION CON KERNEL
    int socket_servidor = iniciar_servidor(ip, config_valores.puerto_escucha_dispatch);

    if (socket_servidor == -1)
    {
        log_info(logger, "Error al iniciar el servidor");
        return EXIT_FAILURE;
    }

	socket_kernel = esperar_cliente(socket_servidor);

	log_info(logger, "Conexion con kernel exitosa");

	//enviar_mensaje("PRUEBA CONEXION CON MEMORIA", conexion);

	liberar_conexion(conexion_memoria);
	config_destroy(config);
	log_destroy(logger);
	return EXIT_SUCCESS;
}

void cargar_configuracion() {
    
	config = config_create("cfg/archivo_configuracion.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

	config_valores.entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
	config_valores.reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
	config_valores.retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	config_valores.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
	config_valores.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");

    log_info(logger, "Termino de leer el archivo de configuracion");
}

