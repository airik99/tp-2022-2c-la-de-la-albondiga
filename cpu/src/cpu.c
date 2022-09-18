#include "cpu.h"

config_cpu config_valores;
t_config* config;
t_log* logger;

int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test")==0)
        return run_tests();
    else{  
        t_log* logger = log_create("./cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
        log_info(logger, "Soy la cpu! %s", mi_funcion_compartida());
        log_destroy(logger);
    } 
    cargar_configuracion(); 
}

void cargar_configuracion() {
    
	config = config_create("cfg/archivo_configuracion.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

	config_valores.entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
	config_valores.reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");
	config_valores.retardo_instruccion = config_get_int_value(config, "RETARDO_INSTRUCCION");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	config_valores.puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
	config_valores.puerto_escucha_dispatch = config_get_int_value(config, "PUERTO_ESCUCHA_DISPATCH");
	config_valores.puerto_escucha_interrupt = config_get_int_value(config, "PUERTO_ESCUCHA_INTERRUPT");

    log_info(logger, "Termino de leer el archivo de configuracion");
    log_destroy(logger);
}