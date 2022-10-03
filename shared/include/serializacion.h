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
 * @brief hola
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
 * @param bytes Tamaño del paquete serializado.
 * 
 * @return La cadena de bytes del paquete serializada
 */
void *serializar_paquete(t_paquete *paquete, int bytes);

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
 * @brief Serializa el array de los segmentos donde se puede leer/escribir
 * 
 * @param paquete_instrucciones Paquete donde se agregan los datos
 * @param segmentos Array de string a serializar 
 */
void serializar_segmentos(t_paquete *paquete_instrucciones, char** segmentos);



void crear_buffer(t_paquete *paquete);

t_instruccion* deserializar_instruccion(void *buffer, int *desplazamiento);

int recibir_operacion(int socket_cliente);

void *recibir_buffer(int *size, int socket_cliente);

t_list *recibir_paquete(int socket_cliente);

t_proceso* recibir_proceso(int socket_cliente);

void enviar_mensaje(char* mensaje, int socket_cliente);

#endif