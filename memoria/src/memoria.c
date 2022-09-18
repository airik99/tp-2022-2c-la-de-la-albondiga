#include "memoria.h"

config_memoria config_valores;
t_config* config;
t_log* logger;

int main(int argc, char ** argv){
    
    /*if(argc > 1 && strcmp(argv[1],"-test")==0)
        return run_tests();
    else{  
        
        log_info(logger, "Soy la memoria! %s", mi_funcion_compartida());
        log_destroy(logger);
    }*/

    logger = log_create("./cfg/memoria.log", "MEMORIA", true, LOG_LEVEL_INFO);

    //PRUEBAS
    cargar_configuracion(); 
}

void cargar_configuracion() {
    
	config = config_create("cfg/archivo_configuracion.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");

	config_valores.entradas_por_tabla = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	config_valores.marcos_por_proceso = config_get_int_value(config, "MARCOS_POR_PROCESO");
	config_valores.path_swap = config_get_string_value(config, "PATH_SWAP");
	config_valores.puerto = config_get_int_value(config, "PUERTO_ESCUCHA");
	config_valores.retardo_memoria = config_get_int_value(config, "RETARDO_MEMORIA");
	config_valores.retardo_swap = config_get_int_value(config, "RETARDO_SWAP");
	config_valores.tam_memoria = config_get_int_value(config, "TAM_MEMORIA");
	config_valores.tam_pagina = config_get_int_value(config, "TAM_PAGINA");
    config_valores.tam_pagina = config_get_int_value(config, "TAMANIO_SWAP");

    log_info(logger, "Termino de leer el archivo de configuracion");
    log_destroy(logger);
}