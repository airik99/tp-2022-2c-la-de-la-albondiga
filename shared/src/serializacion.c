#include <serializacion.h>

int enviar_datos(int socket_fd, void *source, uint32_t size) {
    return send(socket_fd, source, size, 0);
}

int recibir_datos(int socket_fd, void *dest, uint32_t size) {
    return recv(socket_fd, dest, size, 0);  // cuantos bytes a recibir y a donde los quiero recibir
}

t_paquete *crear_paquete(op_code codigo_op) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_op;
    crear_buffer(paquete);
    return paquete;
}

void crear_buffer(t_paquete *paquete) {
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

void *serializar_paquete(t_paquete *paquete, int bytes) {
    void *magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio) {
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio);
    memcpy(paquete->buffer->stream + paquete->buffer->size, valor, tamanio);
    paquete->buffer->size += tamanio;
}

void eliminar_paquete(t_paquete *paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente) {
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
        return cod_op;
    else {
        close(socket_cliente);
        return -1;
    }
}

void *recibir_buffer(int *size, int socket_cliente) {
    void *buffer;

    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}

//-------------------------------------------------------------------------------------------------//
void serializar_segmentos(t_paquete *paquete, char **segmentos) {
    for (int i = 0; segmentos[i] != NULL; i++) {
        int *valor = malloc(sizeof(int));
        int temp_val = atoi(segmentos[i]);
        memcpy(valor, &temp_val, sizeof(int));
        agregar_a_paquete(paquete, valor, sizeof(int));
        free(valor);
    }
}

void serializar_instrucciones(t_paquete *paquete, t_list *instrucciones) {
    int cantidad_instrucciones = list_size(instrucciones);

    agregar_a_paquete(paquete, &cantidad_instrucciones, sizeof(int));
    for (int i = 0; i < cantidad_instrucciones; i++) {
        t_instruccion *instruccion = list_get(instrucciones, i);

        agregar_a_paquete(paquete, &(instruccion->largo_nombre), sizeof(int));
        agregar_a_paquete(paquete, instruccion->nombre, instruccion->largo_nombre);

        agregar_a_paquete(paquete, &(instruccion->cant_params), sizeof(int));
        for (int j = 0; j < instruccion->cant_params; j++) {
            char *parametro = list_get(instruccion->params, j);
            int largo_parametro = strlen(parametro) + 1;
            agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
            agregar_a_paquete(paquete, parametro, largo_parametro);
        }
    }
}

t_list *deserializar_instrucciones(void *buffer, int *desplazamiento) {
    t_list *instrucciones = list_create();
    int contador = 0;
    int cantidad_instrucciones;

    memcpy(&cantidad_instrucciones, buffer + *desplazamiento, sizeof(int));
    *desplazamiento += sizeof(int);

    while (contador < cantidad_instrucciones) {
        t_instruccion *instruccion = malloc(sizeof(t_instruccion));
        instruccion->params = list_create();

        memcpy(&(instruccion->largo_nombre), buffer + *desplazamiento, sizeof(int));
        *desplazamiento += sizeof(int);

        instruccion->nombre = malloc(instruccion->largo_nombre);
        memcpy(instruccion->nombre, buffer + *desplazamiento, instruccion->largo_nombre);
        *desplazamiento += instruccion->largo_nombre;

        memcpy(&(instruccion->cant_params), buffer + *desplazamiento, sizeof(int));
        *desplazamiento += sizeof(int);

        for (int i = 0; i < (instruccion->cant_params); i++) {
            int largo_param;
            memcpy(&largo_param, buffer + *desplazamiento, sizeof(int));
            *desplazamiento += sizeof(int);

            char *param = malloc(largo_param);
            memcpy(param, buffer + *desplazamiento, largo_param);
            *desplazamiento += largo_param;
            list_add(instruccion->params, param);
        }
        list_add(instrucciones, instruccion);
        contador++;
    }
    return instrucciones;
}

t_proceso *recibir_proceso(int socket_cliente) {
    int size;
    int desplazamiento = 0;
    void *buffer;

    t_proceso *proceso_consola = malloc(sizeof(t_proceso));
    proceso_consola->espacios_memoria = list_create();
    buffer = recibir_buffer(&size, socket_cliente);

    proceso_consola->instrucciones = deserializar_instrucciones(buffer, &desplazamiento);

    while (desplazamiento < size) {
        int valor;
        memcpy(&valor, buffer + desplazamiento, sizeof(int));
        list_add(proceso_consola->espacios_memoria, valor);
        desplazamiento += sizeof(int);
    }
    free(buffer);
    return proceso_consola;
}

void serializar_pcb(t_paquete *paquete, t_pcb *pcb) {
    agregar_a_paquete(paquete, &(pcb->pid), sizeof(u_int32_t));
    serializar_instrucciones(paquete, pcb->instrucciones);
    agregar_a_paquete(paquete, &(pcb->socket_consola), sizeof(u_int32_t));
    agregar_a_paquete(paquete, &(pcb->program_counter), sizeof(u_int32_t));
    for (int i = 0; i < 4; i++) {
        agregar_a_paquete(paquete, &(pcb->registro[i]), sizeof(u_int32_t));
    }
    agregar_a_paquete(paquete, &(pcb->estado_actual), sizeof(t_estado));
    agregar_a_paquete(paquete, &(pcb->estado_anterior), sizeof(t_estado));
    // TODO: Tabla de segmentos
}

t_pcb *deserializar_pcb(void *buffer, int *desplazamiento) {
    t_pcb *pcb = malloc(sizeof(t_pcb));
    memcpy(&(pcb->pid), buffer + *desplazamiento, sizeof(int));
    *desplazamiento += sizeof(u_int32_t);
    pcb->instrucciones = deserializar_instrucciones(buffer, desplazamiento);
    memcpy(&(pcb->socket_consola), buffer + *desplazamiento, sizeof(u_int32_t));
    *desplazamiento += sizeof(u_int32_t);
    memcpy(&(pcb->program_counter), buffer + *desplazamiento, sizeof(u_int32_t));
    *desplazamiento += sizeof(u_int32_t);
    for (int i = 0; i < 4; i++) {
        memcpy(&(pcb->registro[i]), buffer + *desplazamiento, sizeof(u_int32_t));
        *desplazamiento += sizeof(u_int32_t);
    }
    memcpy(&(pcb->estado_actual), buffer + *desplazamiento, sizeof(t_estado));
    *desplazamiento += sizeof(t_estado);
    memcpy(&(pcb->estado_anterior), buffer + *desplazamiento, sizeof(t_estado));
    *desplazamiento += sizeof(t_estado);
    return pcb;
}

t_pcb *recibir_pcb(int socket_cliente) {
    int size;
    int desplazamiento = 0;
    void *buffer;

    buffer = recibir_buffer(&size, socket_cliente);
    t_pcb *pcb = deserializar_pcb(buffer, &desplazamiento);
    free(buffer);
    return pcb;
}

t_solicitud_io *recibir_pcb_io(int socket_cliente) {
    int size;
    int desplazamiento = 0;
    void *buffer;
    int largo_nombre, largo_parametro;
    t_solicitud_io *solicitud = malloc(sizeof(t_solicitud_io));

    buffer = recibir_buffer(&size, socket_cliente);
    solicitud->pcb = deserializar_pcb(buffer, &desplazamiento);

    memcpy(&largo_nombre, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    solicitud->dispositivo = malloc(largo_nombre);
    memcpy(solicitud->dispositivo, buffer + desplazamiento, largo_nombre);
    desplazamiento += largo_nombre;

    memcpy(&largo_parametro, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    solicitud->parametro = malloc(largo_parametro);
    memcpy(solicitud->parametro, buffer + desplazamiento, largo_parametro);
    free(buffer);
    return solicitud;
}

void enviar_pcb(t_pcb *pcb, op_code codigo_op, int socket) {
    t_paquete *paquete = crear_paquete(codigo_op);
    serializar_pcb(paquete, pcb);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);
}
