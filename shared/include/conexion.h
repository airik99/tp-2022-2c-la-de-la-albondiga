#ifndef CONEXION_H
#define CONEXION_H

#include <shared_utils.h>
#include <netdb.h>
#include <sys/socket.h>

/**
 * @DESC: Inicia el servidor
 *
 * @PARAMS:
 *  +ip: Ip del servidor
 *  +puerto: Puerto del servidor
 * 
 *
 * @RETURN: Socket del servidor o -1 por error.
 */
int iniciar_servidor(char *ip, char *puerto);

/**
 * @DESC Espera a que un cliente se conecte al servidor.
 *
 * @PARAMS: 
 *   +socketServidor: Socket del servidor.
 *
 * @RETURN: Socket del cliente que se conecta o -1 por error.
 */
int esperar_cliente(int socket_servidor);

/**
 * @DESC: Se conecta a un servidor
 *
 * @PARAMS: 
 * +ip: IP del servidor.
 * +puerto: Puerto del servidor.
 *
 * @RETURN: Socket del cliente o -1 por error.
 */
int conectarse_a_servidor(char *ip, char *puerto);

/**
 * @DESC: Libera la conexión con el servidor.
 *
 * @PARAMS:
 *   +socket_cliente: Socket del cliente.
 */
void liberar_conexion(int socket_cliente);


#endif