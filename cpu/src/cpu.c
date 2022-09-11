#include "cpu.h"
int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test")==0)
        return run_tests();
    else{  
        t_log* logger = log_create("./cfg/cpu.log", "CPU", true, LOG_LEVEL_INFO);
        log_info(logger, "Soy la cpu! %s", mi_funcion_compartida());
        log_destroy(logger);
    } 
}