#include <kernel_utils.h>

config_kernel config_valores;
t_config *config;
t_log *logger;
t_queue *cola_new;
t_queue *cola_exit;
t_queue *cola_exec;
t_queue *cola_ready_prioritaria;
t_queue *cola_ready_segundo_nivel;
t_queue *colaBlock;
char *estado_a_string[] = {"NEW", "READY", "EXEC", "BLOCKED", "EXIT"};
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
    config_valores.algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
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

// esto tiene que hacerlo cuando se conecta una consola al kernel
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
    queue_clean_and_destroy_elements(colaBlock, (void *)eliminar_pcb);
    queue_clean_and_destroy_elements(cola_exit, (void *)eliminar_pcb);
    queue_clean_and_destroy_elements(cola_ready_prioritaria, (void *)eliminar_pcb);
    queue_clean_and_destroy_elements(cola_ready_segundo_nivel, (void *)eliminar_pcb);
    queue_clean_and_destroy_elements(cola_new, (void *)eliminar_pcb);
    queue_clean_and_destroy_elements(cola_exec, (void *)eliminar_pcb);
}

void actualizar_estado(t_pcb *pcb, t_estado nuevo_estado) {
    log_info(logger, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>",
             pcb->pid, estado_a_string[pcb->estado_actual], estado_a_string[nuevo_estado]);
    pcb->estado_anterior = pcb->estado_actual;
    pcb->estado_actual = nuevo_estado;
}

bool es_algoritmo_FEEDBACK() {
    return strcmp(config_valores.algoritmo_planificacion, "FEEDBACK") == 0;
}
bool es_algoritmo_FIFO() {
    return strcmp(config_valores.algoritmo_planificacion, "FIFO") == 0;
}
bool es_algoritmo_RR() {
    return strcmp(config_valores.algoritmo_planificacion, "RR") == 0;
}

void loggear_colas_ready() {
    char *pids_primera_cola = string_de_pids(cola_ready_prioritaria);

    if (es_algoritmo_FEEDBACK) {
        char *pids_segunda_cola = string_de_pids(cola_ready_segundo_nivel);
        log_info(logger, "Cola Ready <Round Robin>: [%s]", pids_primera_cola);
        log_info(logger, "Cola Ready <FIFO>: [%s]", pids_segunda_cola);
    } else {
        log_info(logger, "Cola Ready <%s>: [%s]", config_valores.algoritmo_planificacion, pids_primera_cola);
    }
}

char *string_de_pids(t_queue *cola) {
    t_list *pid_list = list_map(cola->elements, obtener_pid_como_string);
    return list_fold1(pid_list, concatenar_string_con_coma);
}

char *obtener_pid_como_string(t_pcb *pcb) {
    return string_itoa(pcb->pid);
}

char *concatenar_string_con_coma(char *string1, char *string2) {
    char *temp = string_duplicate(string1);
    string_append_with_format(&temp, "%s, %s", string2);
    return temp;
}