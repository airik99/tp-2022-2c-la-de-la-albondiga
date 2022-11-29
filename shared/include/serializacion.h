#ifndef SERIALIZACION_H
#define SERIALIZACION_H

#include <shared_utils.h>
#include <conexion.h>

//estructuras
typedef enum
{
	MENSAJE,
	INSTRUCCIONES,
	HANDSHAKE,
	PCB,
	INTERRUPCION,
	PCB_BLOCK,
	PCB_EXIT,
	IO_TECLADO,
	IO_PANTALLA,
	PAQUETE,
	PAGE_FAULT_REQUEST,
	ESCRITURA_MEMORIA,
	TAM_PAGINA,
	ENTRADAS_POR_TABLA,
	SEGMENTATION_FAULT,
	ESTA_EN_MEMORIA,
	PAGE_FAULT,
	ESCRIBIR_EN_MEMORIA,
	LEER_DE_MEMORIA
} op_code;

typedef struct{
	uint32_t tam_pagina;
	uint32_t entradas;
} t_handshake;

typedef struct
{
	u_int32_t size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

// char** codigo_instrucciones = ["SET", "ADD", "MOV_IN", "MOV_OUT", "I/O DISCO", "EXIT"];

//funciones
/**
 * 
 * @brief Crea un paquete con el codigo de operacion indicado
 * 
 * @param codigo_op El Código de operación del paquete.
 *
 * @return Un paquete.
 */
t_paquete *crear_paquete(op_code codigo_op);

/**
 * @brief Agrega un dato al paquete
 * 
 * @param paquete 
 * @param valor 
 * @param tamanio 
 */
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

/**
 * @brief Serializa el paquete antes de enviarlo
 *
 * 
 * @param paquete Paquete a serializar.
 * @param tamanio Tamaño del paquete serializado.
 * 
 * @return La cadena de bytes del paquete serializada
 */
void *serializar_paquete(t_paquete *paquete, int tamanio);

void enviar_paquete(t_paquete *paquete, int socket_cliente);

/**
 * @brief Elimina un paquete.
 *
 * @param paquete Paquete a eliminar.
 */
void eliminar_paquete(t_paquete *paquete);

/**
 * @brief Serializa una lista de instrucciones 
 *
 * 
 * @param paquete Paquete en el que se envian las instrucciones.
 * @param instrucciones Lista de instrucciones a serializar
 * 
 */
void serializar_instrucciones(t_paquete *paquete, t_list *instrucciones);


/**
 * @brief Crea una lista de instrucciones a partir de los datos del buffer
 * 
 * @param buffer buffer de datos
 * @param desplazamiento puntero a un int con el desplazamiento del buffer
 *
 * @return t_list* 
 */
t_list *deserializar_instrucciones(void *buffer, int *desplazamiento);

/**
 * @brief Serializa el array de los segmentos donde se puede leer/escribir
 * 
 * @param paquete Paquete donde se agregan los datos
 * @param segmentos Array de string a serializar 
 */
void serializar_segmentos(t_paquete *paquete, char** segmentos);


/**
 * @brief Serializa un pcb 
 * @param paquete Paquete donde se agregan los datos
 * @param pcb Pcb a serializar 
 */
void serializar_pcb(t_paquete *paquete, t_pcb* pcb);

t_pcb *deserializar_pcb(void *buffer, int *desplazamiento);


/**
 * @brief Crea un paquete con cierto codigo de operacion, agrega el pcb y lo envia
 * 
 * @param pcb pcb a enviar
 * @param codigo_op codigo de operacion del pcb
 * @param socket destino
 */
void enviar_pcb(t_pcb* pcb, op_code codigo_op, int socket);
/**
 * @brief Recibe un pcb
 * 
 * @param socket_cliente
 * 
 */
t_pcb *recibir_pcb(int socket_cliente) ;

t_solicitud_io* recibir_pcb_io(int socket_cliente);

void crear_buffer(t_paquete *paquete);

int recibir_operacion(int socket_cliente);

void *recibir_buffer(int *size, int socket_cliente);

t_proceso* recibir_proceso(int socket_cliente);

int enviar_datos(int socket_fd, void *source, uint32_t size);

int recibir_datos(int socket_fd, void *dest, uint32_t size);



#endif