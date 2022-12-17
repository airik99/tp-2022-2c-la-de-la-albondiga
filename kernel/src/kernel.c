#include "kernel.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        puts("FALTO CONFIG");
        return EXIT_FAILURE;
    }
    iniciar_logger();
    char *path_config = argv[1];
    cargar_configuracion(path_config);
    pthread_mutex_init(&mx_cantidad_procesos, NULL);
    pthread_mutex_init(&mx_memoria, NULL);
    pthread_mutex_init(&mx_cpu, NULL);

    // CONEXION CON CONSOLAS
    socket_servidor = iniciar_servidor(config_valores.puerto_escucha);

    if (socket_servidor == -1) {
        pthread_mutex_lock(&mx_log);
        log_error(logger, "Error al iniciar el servidor");
        pthread_mutex_unlock(&mx_log);
        destruir_estructuras();
        return EXIT_FAILURE;
    }

    // CONEXION CON MEMORIA
    conexion_memoria = conectarse_a_servidor(config_valores.ip_memoria, config_valores.puerto_memoria);

    if (conexion_memoria == -1) {
        pthread_mutex_lock(&mx_log);
        log_error(logger, "Error en la conexion con memoria. Terminando kernel");
        pthread_mutex_unlock(&mx_log);
        destruir_estructuras();
        return EXIT_FAILURE;
    }
    int id_handshake = 1;
    send(conexion_memoria, &id_handshake, sizeof(int), MSG_WAITALL);
    recv(conexion_memoria, &id_handshake, sizeof(int), MSG_WAITALL);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "Conexion con memoria exitosa");
    pthread_mutex_unlock(&mx_log);

    // CONEXION CON CPU
    conexion_cpu_interrupt = conectarse_a_servidor(config_valores.ip_cpu, config_valores.puerto_cpu_interrupt);
    if (conexion_cpu_interrupt == -1) {
        pthread_mutex_lock(&mx_log);
        log_error(logger, "Error en la conexion por interrupt. Terminando kernel");
        pthread_mutex_unlock(&mx_log);
        destruir_estructuras();
        liberar_conexion(socket_servidor);
        liberar_conexion(conexion_memoria);
        return EXIT_FAILURE;
    }
    recv(conexion_cpu_interrupt, &id_handshake, sizeof(int), MSG_WAITALL);
    
    pthread_mutex_lock(&mx_log);
    log_info(logger, "Conexion con cpu interrupt exitosa");
    pthread_mutex_unlock(&mx_log);
    conexion_cpu_dispatch = conectarse_a_servidor(config_valores.ip_cpu, config_valores.puerto_cpu_dispatch);
    if (conexion_cpu_dispatch == -1) {
        pthread_mutex_lock(&mx_log);
        log_error(logger, "Error en la conexion por dispatch. Terminando kernel");
        pthread_mutex_unlock(&mx_log);
        destruir_estructuras();
        liberar_conexion(socket_servidor);
        liberar_conexion(conexion_memoria);
        liberar_conexion(conexion_cpu_interrupt);
        return EXIT_FAILURE;
    }
    recv(conexion_cpu_dispatch, &id_handshake, sizeof(int), MSG_WAITALL);
    pthread_mutex_lock(&mx_log);
    log_info(logger, "Conexion con cpu dispatch exitosa");
    pthread_mutex_unlock(&mx_log);

    iniciar_planificador_largo_plazo();
    iniciar_planificador_corto_plazo();

    pthread_create(&t_manejo_consola, NULL, (void *)manejar_consolas, (void *)socket_servidor);
    pthread_join(t_manejo_consola, NULL);

    liberar_conexion(socket_servidor);
    liberar_conexion(conexion_memoria);
    liberar_conexion(conexion_cpu_interrupt);
    liberar_conexion(conexion_cpu_dispatch);

    liberar_colas();
    eliminar_semaforos();
    destruir_estructuras();
    return EXIT_SUCCESS;
}

void manejar_consolas(int socket_servidor) {
    while (1) {
        int *socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(socket_servidor);
        //int socket_cliente = esperar_cliente(socket_servidor);
        pthread_t t;
        pthread_create(&t, NULL, (void *)escuchar_consola, *socket_cliente);
        pthread_detach(t);
        free(socket_cliente);
    }
}

void escuchar_consola(int socket_cliente) {
    int cod_op = recibir_operacion(socket_cliente);
    uint32_t respuesta;
    switch (cod_op) {
        case INSTRUCCIONES:
            t_proceso *p = recibir_proceso(socket_cliente);
            t_pcb *pcb = crear_nuevo_pcb(socket_cliente, p->espacios_memoria, p->instrucciones);
            free(p);
            pthread_mutex_lock(&mx_log);
            log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
            pthread_mutex_unlock(&mx_log);
            pushear_semaforizado(cola_new, pcb, &mx_cola_new);
            sem_post(&sem_procesos_new);
            break;
        default:
            respuesta = 1;
            send(socket_cliente, &respuesta, sizeof(uint32_t), 0);
            pthread_mutex_lock(&mx_log);
            log_error(logger, "operacion no valida");
            pthread_mutex_unlock(&mx_log);
            break;
    }
}