#include "kernel.h"

config_kernel config_valores;
t_config *config;
t_log *logger;
char* ip;

int main(int argc, char **argv)
{
    logger = log_create("cfg/kernel.log", "KERNEL", true, LOG_LEVEL_INFO);

    cargar_configuracion();
    ip = "127.0.0.1";

    //CONEXION CON CONSOLAS
    int socket_servidor = iniciar_servidor(ip, config_valores.puerto_escucha);

    if (socket_servidor == -1)
    {
        log_info(logger, "Error al iniciar el servidor");
        return EXIT_FAILURE;
    }
    log_info(logger, "Kernel listo para recibir consolas");

    int socket_cliente = esperar_cliente(socket_servidor);

    log_info(logger, "Consola conectada. Cerrando programa");

    //CONEXION CON MEMORIA
	log_info(logger, "Kernel iniciado. Intentando conectarse con la memoria");

	int conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);

	if (conexion_memoria == -1) {
		log_info(logger, "Error en la conexion al servidor. Terminando kernel");
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion con memoria exitosa");

    //CONEXION CON CPU
    int conexion_cpu = conectarse_a_servidor(ip, config_valores.puerto_cpu_dispatch);

	if (conexion_cpu == -1) {
		log_info(logger, "Error en la conexion al servidor. Terminando kernel");
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion con cpu dispatch exitosa");

    liberar_conexion(conexion_cpu);
    liberar_conexion(conexion_memoria);
	config_destroy(config);
	log_destroy(logger);
    
    return EXIT_SUCCESS;
}

void cargar_configuracion()
{

    config = config_create("cfg/Kernel.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores.ip_cpu = config_get_string_value(config, "IP_CPU");
    config_valores.puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    config_valores.puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    config_valores.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    config_valores.algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    config_valores.grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
    config_valores.dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
    config_valores.tiempos_io = config_get_array_value(config, "TIEMPOS_IO");
    config_valores.quantum_rr = config_get_int_value(config, "QUANTUM_RR");

    log_info(logger, "Termino de leer el archivo de configuracion");
    //log_destroy(logger);
}