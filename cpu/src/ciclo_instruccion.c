#include <ciclo_instruccion.h>

void ciclo_de_instruccion(t_pcb* pcb) {
    t_instruccion* instruccionProxima;
    flag_salida = 0;
    interrupcion = 0;

    while (pcb->program_counter < list_size(pcb->instrucciones)) {
        instruccionProxima = list_get(pcb->instrucciones, pcb->program_counter);  // FETCH
        pcb->program_counter++;
        decode(instruccionProxima, pcb);  // DECODE (CON EXECUTE INCLUIDO)
        if (flag_salida) {
            return;
        }
        if (check_interrupt()) {  // SE FIJA QUE NO HAYA UNA INTERRUPCION ANTES DE SEGUIR CON EL CICLO DE INSTRUCCION
            ultimo_pid = pcb->pid;
            t_paquete* paquete = crear_paquete(INTERRUPCION);
            serializar_pcb(paquete, pcb);
            enviar_paquete(paquete, cliente_servidor_dispatch);
            return;
        }
    }
}

void decode(t_instruccion* instruccion, t_pcb* pcb) {
    if (strcmp(instruccion->nombre, "SET") == 0) {
        char* registro = list_get(instruccion->params, 0);
        uint32_t valor = atoi(list_get(instruccion->params, 1));
        log_info(logger, "PID: <%d> - Ejecutando: <SET> - <%s> - <%d>\n", pcb->pid, registro, valor); //log obligatorio
        ejecutar_SET(registro, valor);
    }

    else if (strcmp(instruccion->nombre, "ADD") == 0) {
        char* destino = list_get(instruccion->params, 0);
        char* origen = list_get(instruccion->params, 1);
        log_info(logger, "PID: <%d> - Ejecutando: <ADD> - <%s> - <%s>\n", pcb->pid, destino, origen); //log obligatorio
        ejecutar_ADD(destino, origen);
    }

    else if (strcmp(instruccion->nombre, "MOV_IN") == 0) {
        uint32_t direccion_logica = list_get(instruccion->params, 1);
        char* registro = list_get(instruccion->params, 0);
        pcb_actual = pcb;
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_IN> - <%d> - <%s>\n", pcb->pid, direccion_logica, registro); //log obligatorio
        ejecutar_MOV_IN(registro, direccion_logica); 
    }

    else if (strcmp(instruccion->nombre, "MOV_OUT") == 0) {
        uint32_t direccion_logica = list_get(instruccion->params, 0);
        char* registro = list_get(instruccion->params, 1);
        pcb_actual = pcb;
        log_info(logger, "PID: <%d> - Ejecutando: <MOV_OUT> - <%d> - <%s>\n", pcb->pid, direccion_logica, registro); //log obligatorio
        ejecutar_MOV_OUT(direccion_logica, registro);
    }

    else if (strcmp(instruccion->nombre, "I/O") == 0) {
        char* dispositivo = list_get(instruccion->params, 0);
        char* param2 = list_get(instruccion->params, 1);
        log_info(logger, "PID: <%d> - Ejecutando: <I/O> - <%s> - <%s>\n", pcb->pid, dispositivo, param2); //log obligatorio
        ejecutar_IO(dispositivo, param2, pcb);
    }

    else if (strcmp(instruccion->nombre, "EXIT") == 0) {
        log_info(logger, "PID: <%d> - Ejecutando: <EXIT> -\n", pcb->pid);
        ejecutar_EXIT(pcb);
    } else {
        log_error(logger, "PID: <%d> - INSTRUCCION NO RECONOCIDA\n", pcb->pid);
    }
}

// SET (Registro, Valor): Asigna al registro el valor pasado como parámetro.
void ejecutar_SET(char* registro, uint32_t valor) {
    usleep(config_valores.retardo_instruccion * 1000);
    int indice = indice_registro(registro);
    registros[indice] = valor;
    log_info(logger, "Se guarda el valor %d en el registro %s \n", valor, registro);
}

// ADD (Registro Destino, Registro Origen): Suma ambos registros y deja el resultado en el Registro Destino.
void ejecutar_ADD(char* destino, char* origen) {
    usleep(config_valores.retardo_instruccion * 1000);
    int registro_origen = indice_registro(origen);
    int registro_destino = indice_registro(destino);
    int resultado = registros[registro_destino] + registros[registro_origen];
    log_info(logger, "La suma entre %s (%d) y %s (%d) es %d \n", destino, registros[registro_destino], origen, registros[registro_origen], resultado);
    registros[registro_destino] = resultado;
}

// I/O (Dispositivo, Registro / Unidades de trabajo): Esta instrucción representa una syscall de I/O bloqueante. Se deberá devolver
// el Contexto de Ejecución actualizado al Kernel junto el dispositivo y la cantidad de unidades de trabajo del dispositivo que desea
// utilizar el proceso (o el Registro a completar o leer en caso de que el dispositivo sea Pantalla o Teclado).
void ejecutar_IO(char* dispositivo, char* parametro, t_pcb* pcb) {
    copiar_valores_registros(registros, (pcb->registro));
    t_paquete* paquete = crear_paquete(PCB_BLOCK);
    int largo_nombre = strlen(dispositivo) + 1;
    int largo_parametro = strlen(parametro) + 1;
    serializar_pcb(paquete, pcb);
    agregar_a_paquete(paquete, &largo_nombre, sizeof(int));
    agregar_a_paquete(paquete, dispositivo, largo_nombre);
    agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
    agregar_a_paquete(paquete, parametro, largo_parametro);
    enviar_paquete(paquete, cliente_servidor_dispatch);  // dispatch
    eliminar_paquete(paquete);
    flag_salida = 1;
}

// MOV_IN (Registro, Dirección Lógica): Lee el valor de memoria del segmento de Datos correspondiente a la Dirección Lógica y lo almacena en el Registro.
void ejecutar_MOV_IN(char* registro, uint32_t direccion_logica) {
    uint32_t direccion_fisica = traducir_direccion_logica(direccion_logica);
    uint32_t valor = leer_de_memoria(direccion_fisica);
    int indice = indice_registro(registro);
    registros[indice] = valor;
    log_info(logger, "Se guarda el valor %d en el registro %s \n", valor, registro);  // hay que ver si devuelve un numero o el enum en sí
}

uint32_t leer_de_memoria(uint32_t direccion_fisica) {
    t_paquete* paquete = crear_paquete(LEER_DE_MEMORIA); //TODO: aca hay que acordarnos de evaluar este cod_op en memoria
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    t_paquete* paquete_recibido = recibir_paquete(conexion_memoria);
    uint32_t valor; //= aca tenemos que recibir el valor de memoria
    eliminar_paquete(paquete_recibido);
    return valor;
}

// MOV_OUT (Dirección Lógica, Registro): Lee el valor del Registro y lo escribe en la dirección física de memoria del segmento de Datos obtenida a partir
// de la Dirección Lógica.
void ejecutar_MOV_OUT(uint32_t direccion_logica, char* registro) {
    uint32_t direccion_fisica = traducir_direccion_logica(direccion_logica);
    int indice = indice_registro(registro);
    uint32_t valor = registros[indice];
    escribir_en_memoria(direccion_fisica, valor);
}

void escribir_en_memoria(uint32_t direccion_fisica, uint32_t valor) {
    t_paquete* paquete = crear_paquete(ESCRIBIR_EN_MEMORIA); //TODO: aca hay que acordarnos de evaluar este cod_op en memoria
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &valor, sizeof(uint32_t));
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);
}

// EXIT Esta instrucción representa la syscall de finalización del proceso. Se deberá devolver el PCB actualizado al Kernel para su finalización.
void ejecutar_EXIT(t_pcb* pcb) {
    // SOLICITUD MEMORIA
    copiar_valores_registros(registros, (pcb->registro));
    enviar_pcb(pcb, PCB_EXIT, cliente_servidor_dispatch);
    flag_salida = 1;
}

int check_interrupt() {
    return interrupcion;
}