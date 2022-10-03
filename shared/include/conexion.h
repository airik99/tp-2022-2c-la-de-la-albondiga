#ifndef CONEXION_H
#define CONEXION_H

#include <shared_utils.h>
#include <netdb.h>
#include <sys/socket.h>

/**
 * @brief Inicia el servidor
 *
 * @param Ip ip del servidor
 * @param Puerto puerto del servidor
 * 
 *
 * @return Socket del servidor o -1 por error.
 **/
int iniciar_servidor(char *ip, char *puerto);

/**
 * @brief Espera a que un cliente se conecte al servidor.
 *
 * @param Socket socket del servidor.
 *
 * @return Socket del cliente que se conecta o -1 por error.
 */
int esperar_cliente(int socket_servidor);

/**
 * @brief Se conecta a un servidor
 *
 * @param IP ip del servidor.
 * @param Puerto puerto del servidor.
 *
 * @return Socket del cliente o -1 por error.
 */
int conectarse_a_servidor(char *ip, char *puerto);

/**
 * @brief Libera la conexión con el servidor.
 *
 * @param socket_cliente Socket del cliente.
 */
void liberar_conexion(int socket_cliente);


#endif