#include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;
char* ip;

int main(int argc, char ** argv){

    logger = log_create("cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
    ip = "127.0.0.1";
	//Conexion a memoria - cliente
	
    cargar_configuracion(); 

    log_info(logger, "Cpu iniciado. Intentando conectarse con la memoria");

    int conexion = conectarse_a_servidor(ip, config_valores.puerto_memoria);

    log_info(logger, "Conexion correcta. Enviando mensaje");
	enviar_mensaje("PRUEBA CONEXION CON MEMORIA", conexion);

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

