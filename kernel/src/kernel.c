#include "kernel.h"

char *ip;
pthread_t t_manejo_consola;

void manejador_seniales(int senial) {
    switch (senial) {
        case SIGINT:
            log_info(logger, "Cerrando hilos");
            pthread_cancel(t_manejo_consola);
            break;
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, manejador_seniales);
    iniciar_logger();
    cargar_configuracion();
    iniciar_planificador_largo_plazo();
    iniciar_planificador_corto_plazo();

    ip = "127.0.0.1";  // esto no debería estar hardcodeado

    // CONEXION CON CONSOLAS
    socket_servidor = iniciar_servidor(ip, config_valores.puerto_escucha);

    if (socket_servidor == -1) {
        log_info(logger, "Error al iniciar el servidor");
        return EXIT_FAILURE;
    }
    log_info(logger, "Kernel listo para recibir consolas");

    /*
    // CONEXION CON MEMORIA
    log_info(logger, "Kernel iniciado. Intentando conectarse con la memoria");

    conexion_memoria = conectarse_a_servidor(ip, config_valores.puerto_memoria);

    if (conexion_memoria == -1) {
        log_info(logger, "Error en la conexion al servidor. Terminando kernel");
        return EXIT_FAILURE;
    }

    log_info(logger, "Conexion con memoria exitosa");*/

    // CONEXION CON CPU
    conexion_cpu_interrupt = conectarse_a_servidor(ip, config_valores.puerto_cpu_interrupt);
    log_info(logger, "Conexion con cpu interrupt exitosa");
    conexion_cpu_dispatch = conectarse_a_servidor(ip, config_valores.puerto_cpu_dispatch);
    log_info(logger, "Conexion con cpu dispatch exitosa");
    if (conexion_cpu_dispatch == -1 || conexion_cpu_interrupt == -1) {
        log_info(logger, "Error en la conexion al servidor. Terminando kernel");
        return EXIT_FAILURE;
    }

    pthread_create(&t_manejo_consola, NULL, (void *)manejar_consolas, (void *)socket_servidor);
    pthread_join(t_manejo_consola, NULL);

    liberar_conexion(conexion_cpu_dispatch);
    liberar_conexion(conexion_cpu_interrupt);
    //  liberar_conexion(conexion_memoria);

    liberar_colas();
    destruir_estructuras();
    // eliminar_paquete(paquete);
    return EXIT_SUCCESS;
}

void manejar_consolas(int socket_servidor) {
    while (1) {
        int socket_cliente = esperar_cliente(socket_servidor);
        // se crea thread por cada consola
        pthread_t t;
        pthread_create(&t, NULL, (void *)escuchar_consola, (void *)socket_cliente);
        pthread_join(t, NULL);
    }
}

void escuchar_consola(int socket_cliente) {
    log_info(logger, "Se conecto una consola");
    int cod_op = recibir_operacion(socket_cliente);
    t_proceso *proceso;
    uint32_t respuesta;
    switch (cod_op) {
        case INSTRUCCIONES:
            proceso = recibir_proceso(socket_cliente);
            t_pcb *pcb = crear_nuevo_pcb(proceso);
            log_info(logger, "Se crea el proceso <%d> en NEW", pcb->pid);
            pthread_mutex_lock(&mx_cola_new);
            queue_push(cola_new, pcb);
            pthread_mutex_unlock(&mx_cola_new);
            sem_post(&sem_procesos_new);
            //  Por ahora borro estas cosas para que no salten MemLeaks con Valgrind despues hay
            list_destroy(proceso->espacios_memoria);
            // free(proceso);
            break;
        default:
            respuesta = 1;
            send(socket_cliente, &respuesta, sizeof(uint32_t), 0);
            log_error(logger, "operacion no valida");
            break;
    }
}