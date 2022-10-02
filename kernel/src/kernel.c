#include "kernel.h"
#include "serializacion.h"

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
    
    log_info(logger, "Se conecto una consola"); 
    int cod_op = recibir_operacion(socket_cliente);
    t_proceso* proceso;
	switch(cod_op){
		case INSTRUCCIONES:
            proceso = recibir_proceso(socket_cliente);
            log_info(logger, "Conexion con consola exitosa");
            break;
		default:
			log_error(logger, "operacion no valida");
			break;
	}

    list_iterate(proceso->instrucciones, destructor_instrucciones);
/*
    //CONEXION CON MEMORIA
	log_info(logger, "Kernel iniciado. Intentando conectarse con la memoria");

	int conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);

	if (conexion_memoria == -1) {
		log_info(logger, "Error en la conexion al servidor. Terminando kernel");
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion con memoria exitosa");

    //CONEXION CON CPU
    int conexion_cpu_dispatch = conectarse_a_servidor(ip, config_valores.puerto_cpu_dispatch);
    log_info(logger, "Conexion con cpu dispatch exitosa");
    
    // int conexion_cpu_interrupt = conectarse_a_servidor(ip, config_valores.puerto_cpu_interrupt);
    // log_info(logger, "Conexion con cpu interrupt exitosa");

	if (conexion_cpu_dispatch == -1 || conexion_cpu_interrupt == -1) {
		log_info(logger, "Error en la conexion al servidor. Terminando kernel");
		return EXIT_FAILURE;
	}

    
 
    liberar_conexion(conexion_cpu_dispatch);
    // liberar_conexion(conexion_cpu_interrupt);
    liberar_conexion(conexion_memoria);*/
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
/*
t_list *deserializar_instrucciones(t_list *stream_datos, uint32_t size_stream_datos) {
	
    t_list *instrucciones = list_create();
    void *magic = malloc(size_stream_datos);
    int desplazamiento = 0;

  	for(int i = 0; i < size_stream_datos; i++) {
    //aca deberia iterar sobre el stream de datos e ir llenando a la lista de instrucciones
    	memcpy(magic + desplazamiento, stream_datos, size_stream_datos);
	    desplazamiento += size_stream_datos;
  	}
  	return instrucciones; 
}
*/

//t_paquete_deserializado *deserializar_consola(int  socket_cliente) {

	// t_paquete *paquete = recibir_paquete(socket_cliente);
  	// t_paquete_deserializado *paquete_deserializado = malloc(sizeof(t_paquete_deserializado));
    // //verificar la estructura de t_paquete para deserializarlo
  	// paquete_deserializado->tamanio_proceso = (uint32_t)paquete->buffer->size;
  	// paquete_deserializado->instrucciones = deserializar_instrucciones(paquete->buffer->stream, paquete->buffer->size);
  	// return paquete_deserializado;
//}

