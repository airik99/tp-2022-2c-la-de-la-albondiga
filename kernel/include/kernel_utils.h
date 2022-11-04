#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include "shared_utils.h"

typedef enum {
    RR,
    FIFO,
    FEEDBACK
} t_algoritmo_planificacion;

typedef struct {
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *puerto_escucha;
    t_algoritmo_planificacion algoritmo_planificacion;
    int grado_max_multiprogramacion;
    int quantum_rr;
    char **dispositivos_io;
    char **tiempos_io;
} config_kernel;

// variables
extern config_kernel config_valores;
extern t_config *config;
extern t_log *logger;
extern int contador_pid;
extern t_list *colaNew;
extern t_list *colaExit;
extern t_list *colaReadyFifo;
extern t_list *colaReadyRoundRobin;
extern t_list *colaExec;
extern t_list *colaBlock;
extern int conexion_cpu_dispatch;
extern int conexion_memoria;
extern int conexion_cpu_interrupt;
extern int socket_servidor;


/**
 * @brief Crea el logger del kernel
 *
 */
void iniciar_logger();

/**
 * @brief Carga los valores del archivo config
 *
 */
void cargar_configuracion();

/**
 * @brief Libera memoria asignada a logs, config y otras estructuras
 *
 */
void destruir_estructuras();

/**
 * @brief Crea un pcb
 *
 * @param proceso recibe un proceso que mando una consola
 *
 * @return El pcb de un proceso.
 */
t_pcb *crear_nuevo_pcb(t_proceso *proceso);

/**
 * @brief Libera todos los campos de un pcb
 *
 * @param pcb pcb a eliminar
 */
void eliminar_pcb(t_pcb *pcb);

/**
 * @brief Destruye todas las colas y los pcb que tengan
 *
 */
void liberar_colas();

bool es_algoritmo_FIFO();



#endif