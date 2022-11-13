#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include "shared_utils.h"
#include <signal.h>

typedef struct {
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    int grado_max_multiprogramacion;
    int quantum_rr;
    char **dispositivos_io;
    char **tiempos_io;
} config_kernel;


typedef struct {
    char *dispositivo;
    int tiempo_dispositivo;
    pthread_t t_bloqueo;
    sem_t procesos_bloqueado;
    pthread_mutex_t mx_cola_bloqueados;
    t_queue* cola_bloqueados;
} t_cola_bloqueo;


// variables
extern config_kernel config_valores;
extern t_config *config;
extern t_log *logger;
extern int contador_pid;
extern int conexion_cpu_dispatch;
extern int conexion_memoria;
extern int conexion_cpu_interrupt;
extern int socket_servidor;
extern char *estado_a_string[5];

extern t_queue *cola_new, *cola_ready_prioritaria, *cola_ready_segundo_nivel;
extern t_list *lista_colas_bloqueo;
extern pthread_mutex_t mx_cola_new, mx_cola_ready_prioritaria, mx_cola_ready_segunda;
extern pthread_t t_page_fault, t_quantum, t_largo_plazo, t_corto_plazo, t_manejo_consola;
extern sem_t sem_procesos_new, sem_procesos_ready, sem_grado_multiprogramacion, sem_page_fault;

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
 * @param proceso proceso que mando una consola
 * @param socket socket de la consola que manda el paquete
 *
 * @return El pcb de un proceso.
 */
t_pcb *crear_nuevo_pcb(t_proceso *proceso, int socket);

/**
 * @brief Destruye todas las colas y los pcb que tengan
 *
 */
void liberar_colas();

void destruir_cola_bloqueo();
/**
 * @brief Free de los campos dispositivo, parametro y de la solicitud pero mantiene el pcb, se usa cuando un proceso termino su I/O
 * 
 * @param solicitud 
 */
void liberar_solicitud(t_solicitud_io* solicitud); 
/**
 * @brief Hace free de los campos dispositivo, parametro y de la solicitud pero tambien destruye el pcb asociado.
 * Solo se usa cuando se termina la ejecucion del kernel para destruir cualquier elemento que puede haber quedado en la cola
 * 
 * @param solicitud 
 */
void destruir_solicitud_bloqueo(t_solicitud_io* solicitud); 

/**
 * @brief Cierra todos los hilos al enviar SIGINT (ctrl+c en la consola) para poder liberar todas las estructuras y conexiones correctamente
 *
 * @param senial
 */
void manejador_seniales(int senial);

void actualizar_estado(t_pcb *, t_estado);

bool es_algoritmo_FEEDBACK();
bool es_algoritmo_FIFO();
bool es_algoritmo_RR();

int *pid_array(t_queue *cola);

void loggear_colas_ready();
void eliminar_semaforos();

char *string_de_pids(t_queue *cola);
char *obtener_pid_como_string(t_pcb *pcb);
char *concatenar_string_con_coma(char *string1, char *string2);

#endif