#include "kernel.h"

int main(int argc, char **argv) {
    iniciar_logger();
    cargar_configuracion();
    signal(SIGINT, manejador_seniales);

    // CONEXION CON CONSOLAS
    socket_servidor = iniciar_servidor(config_valores.puerto_escucha);

    if (socket_servidor == -1) {
        log_error(logger, "Error al iniciar el servidor");
        destruir_estructuras();
        return EXIT_FAILURE;
    }
    log_info(logger, "Kernel listo para recibir consolas");

    // CONEXION CON MEMORIA
    log_info(logger, "Kernel iniciado. Intentando conectarse con la memoria");

    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);

    if (conexion_memoria == -1) {
        log_error(logger, "Error en la conexion al servidor. Terminando kernel");
        log_destroy(logger);
        return EXIT_FAILURE;
    }

    log_info(logger, "Conexion con memoria exitosa");

    // CONEXION CON CPU
    conexion_cpu_interrupt = conectarse_a_servidor(config_valores.ip_cpu, config_valores.puerto_cpu_interrupt);
    if (conexion_cpu_interrupt == -1) {
        log_error(logger, "Error en la conexion al por interrupt. Terminando kernel");
        destruir_estructuras();
        liberar_conexion(conexion_memoria);
        return EXIT_FAILURE;
    }
    log_info(logger, "Conexion con cpu interrupt exitosa");
    conexion_cpu_dispatch = conectarse_a_servidor(config_valores.ip_cpu, config_valores.puerto_cpu_dispatch);
    if (conexion_cpu_dispatch == -1) {
        log_error(logger, "Error en la conexion al por interrupt. Terminando kernel");
        destruir_estructuras();
        liberar_conexion(conexion_memoria);
        liberar_conexion(conexion_cpu_interrupt);
        return EXIT_FAILURE;
    }
    log_info(logger, "Conexion con cpu dispatch exitosa");

    iniciar_planificador_largo_plazo();
    iniciar_planificador_corto_plazo();

    pthread_create(&t_manejo_consola, NULL, (void *)manejar_consolas, (void *)socket_servidor);
    pthread_join(t_manejo_consola, NULL);

    liberar_conexion(conexion_memoria);

    liberar_colas();
    eliminar_semaforos();
    destruir_estructuras();
    return EXIT_SUCCESS;
}

void manejar_consolas(int socket_servidor) {
    while (1) {
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor);
        // se crea thread por cada consola
        pthread_t t;
        pthread_create(&t, NULL, (void *)escuchar_consola, *socket_cliente);
        pthread_detach(t);
    }
}

void escuchar_consola(int socket_cliente) {
    log_info(logger, "Se conecto una consola");
    int cod_op = recibir_operacion(socket_cliente);
    uint32_t respuesta;
    switch (cod_op) {
        case INSTRUCCIONES:
            t_proceso *p = recibir_proceso(socket_cliente);
            t_pcb *pcb = crear_nuevo_pcb(socket_cliente, p->espacios_memoria, p->instrucciones);
            free(p);
            log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
            pthread_mutex_lock(&mx_cola_new);
            queue_push(cola_new, pcb);
            pthread_mutex_unlock(&mx_cola_new);
            sem_post(&sem_procesos_new);
            break;
        default:
            respuesta = 1;
            send(socket_cliente, &respuesta, sizeof(uint32_t), 0);
            log_error(logger, "operacion no valida");
            break;
    }
}