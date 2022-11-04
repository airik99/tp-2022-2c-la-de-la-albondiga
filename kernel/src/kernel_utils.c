#include <kernel_utils.h>

config_kernel config_valores;
t_config *config;
t_log *logger;
t_list *colaNew;
t_list *colaExit;
t_list *colaReady;
t_list *colaExec;
t_list *colaReadyFifo;
t_list *colaReadyRoundRobin;
t_list *colaBlock;
int conexion_cpu_dispatch;
int conexion_memoria;
int conexion_cpu_interrupt;
int socket_servidor;
int contador_pid;

void iniciar_logger() {
    logger = log_create("cfg/kernel.log", "KERNEL", true, LOG_LEVEL_INFO);
}

void cargar_configuracion() {
    config = config_create("cfg/Kernel.config");
    log_info(logger, "Arranco a leer el archivo de configuracion");
    config_valores.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores.ip_cpu = config_get_string_value(config, "IP_CPU");
    config_valores.puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    config_valores.puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    config_valores.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    char * algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    if (strcmp(algoritmo, "FIFO") == 0) {
        config_valores.algoritmo_planificacion = FIFO;
    } else if (strcmp(algoritmo, "RR") == 0) {
        config_valores.algoritmo_planificacion = RR;
    } else if (strcmp(algoritmo, "FEEDBACK") == 0) {
        config_valores.algoritmo_planificacion = FEEDBACK;
    } else {
        log_error(logger, "No se reconoce el algoritmo de planificacion");
    }
    config_valores.grado_max_multiprogramacion = config_get_int_value(config, "GRADO_MAX_MULTIPROGRAMACION");
    config_valores.dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
    config_valores.tiempos_io = config_get_array_value(config, "TIEMPOS_IO");
    config_valores.quantum_rr = config_get_int_value(config, "QUANTUM_RR");

    log_info(logger, "Termino de leer el archivo de configuracion");
}

void destruir_estructuras() {
    config_destroy(config);
    log_destroy(logger);
    string_array_destroy(config_valores.dispositivos_io);
    string_array_destroy(config_valores.tiempos_io);
}

//esto tiene que hacerlo cuando se conecta una consola al kernel
t_pcb *crear_nuevo_pcb(t_proceso *proceso_consola) {
    t_pcb *nuevo_pcb = malloc(sizeof(t_pcb));
    nuevo_pcb->pid = contador_pid;
    contador_pid++;
    nuevo_pcb->instrucciones = proceso_consola->instrucciones;
    nuevo_pcb->program_counter = 0;
    nuevo_pcb->estado_actual = NEW;
    nuevo_pcb->estado_anterior = NEW;
    return nuevo_pcb;
}

void eliminar_pcb(t_pcb *pcb) {
    destructor_instrucciones(pcb->instrucciones);
    free(pcb);
}

void liberar_colas() {
    list_destroy_and_destroy_elements(colaBlock, (void *)eliminar_pcb);
    list_destroy_and_destroy_elements(colaExit, (void *)eliminar_pcb);
    list_destroy_and_destroy_elements(colaReady, (void *)eliminar_pcb);
    list_destroy_and_destroy_elements(colaNew, (void *)eliminar_pcb);
    list_destroy_and_destroy_elements(colaExec, (void *)eliminar_pcb);
}

bool es_algoritmo_FIFO(){
    return config_valores.algoritmo_planificacion == FIFO;
}