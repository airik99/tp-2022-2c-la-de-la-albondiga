#include <consola.h>
t_log *logger;
t_config *config;
t_config_consola config_consola;
int main(int argc, char **argv) {
    if (argc < 3)
        return EXIT_FAILURE;
    char *path_config= argv[1];
    char *path_pseudocodigo= argv[2];

    logger = log_create("cfg/Consola.log", "Consola", true, LOG_LEVEL_INFO);
    iniciar_config(path_config);
    t_list *instrucciones = leer_archivo(path_pseudocodigo);

    log_info(logger, "Consola iniciada. Intentando conectarse al kernel");

    int socket_cliente = conectarse_a_servidor(config_consola.ip_kernel, config_consola.puerto_kernel);

    if (socket_cliente == -1) {
        log_info(logger, "Error en la conexion al servidor. Terminando consola");
        return EXIT_FAILURE;
    }

    /*recv(socket_cliente,&respuesta,sizeof(uint32_t),MSG_WAITALL);
            if(respuesta == 0)log_info(logger, "Finalizacion exitosa");
            else			  log_error(logger, "Ocurrio un error con la ejecucion del proceso");
    */

    t_paquete *paquete_instrucciones = crear_paquete(INSTRUCCIONES);
    serializar_instrucciones(paquete_instrucciones, instrucciones);
    serializar_segmentos(paquete_instrucciones, config_consola.segmentos);

    log_info(logger, "Conexion correcta. Enviando instrucciones");
    list_iterate(instrucciones, (void *)print_instruccion);
    enviar_paquete(paquete_instrucciones, socket_cliente);
    esperar_respuesta(socket_cliente);

    liberar_conexion(socket_cliente);
    destructor_instrucciones(instrucciones);
    eliminar_paquete(paquete_instrucciones);
    string_array_destroy(config_consola.segmentos);
    free(config_consola.ip_kernel);
    free(config_consola.puerto_kernel);
    config_destroy(config);
    log_destroy(logger);
    return EXIT_SUCCESS;
}

t_config *iniciar_config(char *path) {
    config = config_create(path);
    config_consola.ip_kernel = config_get_string_value(config, "IP_KERNEL");
    config_consola.puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    config_consola.segmentos = config_get_array_value(config, "SEGMENTOS");
    config_consola.tiempo_pantalla = config_get_int_value(config, "SEGMENTOS");
}

t_list *leer_archivo(char *nombre) {
    FILE *puntero;
    char *linea_leida;
    puntero = fopen(nombre, "r");
    t_list *lista_instrucciones = list_create();

    if (!puntero) {
        printf("No se pudo leer el archivo \n");
        exit(1);
    }

    while (!feof(puntero)) {
        linea_leida = leer_linea(puntero);
        char **valores = string_split(linea_leida, " ");
        char *nombre = strdup(valores[0]);
        t_list *params = list_create();
        for (int i = 1; valores[i] != NULL; i++) {
            char *param = strdup(valores[i]);
            list_add(params, param);
        }
        t_instruccion *instruccion = crear_instruccion(nombre, params);
        list_add(lista_instrucciones, instruccion);
        free(linea_leida);
        string_array_destroy(valores);
    }
    txt_close_file(puntero);
    return lista_instrucciones;
}

// Lee una linea del archivo y la retorna como char*
char *leer_linea(FILE *puntero) {
    ssize_t bytes_leidos;
    size_t numero_bytes;
    char *cadena;

    numero_bytes = 0;
    cadena = NULL;
    bytes_leidos = getline(&cadena, &numero_bytes, puntero);
    if (bytes_leidos == -1) {
        puts("Error al leer la linea del archivo");
    }
    // saca el newline a las cadenas que lo tengan
    if (string_ends_with(cadena, "\n")) {
        cadena[strlen(cadena) - 1] = '\0';
    }
    return cadena;
}

t_instruccion *crear_instruccion(char *nombre, t_list *params) {
    t_instruccion *instruccion = malloc(sizeof(t_instruccion));
    instruccion->largo_nombre = strlen(nombre) + 1;
    instruccion->nombre = nombre;
    instruccion->cant_params = list_size(params);
    instruccion->params = params;
    return instruccion;
}

void esperar_respuesta(int socket) {
    op_code codigo_operacion;
    int valor;
    int respuesta = 0;
    int size;
    void *buffer;
    do {
        codigo_operacion = recibir_operacion(socket);
        switch (codigo_operacion) {
            case IO_PANTALLA:
                buffer = recibir_buffer(&size, socket);
                memcpy(&valor, buffer, sizeof(int));
                log_info(logger, "Valor del registro: %d", valor);
                usleep(config_consola.tiempo_pantalla * 1000);
                send(socket, &respuesta, sizeof(uint32_t), MSG_WAITALL);
                break;
            case IO_TECLADO:
                log_info(logger, "Ingrese un entero:");
                scanf("%d", &respuesta);
                send(socket, &respuesta, sizeof(uint32_t), MSG_WAITALL);
                break;
            case PCB_EXIT:
                log_info(logger, "Ejecucion finalizada");
                break;
            default:
                log_error(logger, "Error en la respuesta del kernel");
                break;
        }
    } while (codigo_operacion != PCB_EXIT);
}