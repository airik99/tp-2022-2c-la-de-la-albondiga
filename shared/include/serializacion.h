#ifndef SERIALIZACION_H
#define SERIALIZACION_H

#include <shared_utils.h>
#include <conexion.h>

//estructuras
typedef enum
{
	MENSAJE,
	INSTRUCCIONES
} op_code;

typedef struct
{
	uint32_t size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

typedef struct
{
    uint32_t tamanio_proceso;
    t_list* instrucciones;
} t_paquete_deserializado;

typedef struct {
    char* codigo ;
    uint32_t* parametros;
}instruccion;


// char** codigo_instrucciones = ["SET", "ADD", "MOV_IN", "MOV_OUT", "I/O DISCO", "EXIT"];

//funciones
/**
 * @DESC: Crea un paquete con el codigo de operacion indicado
 *
 * @PARAMS:
 *     +codigo_op: El Código de operación del paquete.
 *
 * @RETURN: Un paquete.
 */
t_paquete *crear_paquete(op_code codigo_op);

/**
 * @DESC: Agrega un dato a un paquete.
 *
 * @PARAMS:
 *          +paquete: Paquete a modificar.
 *          +valor: Valor a agregar al paquete.
 *          +tamanio: Tamaño del dato a agregar.
 */
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);

/**
 * @DESC: Serializa el paquete antes de enviarlo
 *
 * @PARAMS:
 *          +paquete: Paquete a serializar.
 *          +bytes: Tamaño del paquete serializado.
 * 
 * @RETURN: La cadena de bytes del paquete serializada
 */
void *serializar_paquete(t_paquete *paquete, int bytes);

/**
 * @DESC:Elimina un paquete.
 *
 * @PARAMS:
 *          +paquete: Paquete a eliminar.
 */
void eliminar_paquete(t_paquete *paquete);

/**
 * @DESC:Serializa una lista de instrucciones y los segmentos donde se puede leer/escribir
 *
 * @PARAMS:
 *          +paquete: Paquete en el que se envian las instrucciones.
 * 			+instrucciones: Lista de instrucciones a serializar
 * 			+segmentos: Array de segmentos
 */
void serializar_instrucciones(t_paquete *paquete, t_list *instrucciones/*, char **segmentos*/);

void crear_buffer(t_paquete *paquete);

int recibir_operacion(int socket_cliente);

void *recibir_buffer(int *size, int socket_cliente);

t_list *recibir_paquete(int socket_cliente);

void enviar_mensaje(char* mensaje, int socket_cliente);

#endif