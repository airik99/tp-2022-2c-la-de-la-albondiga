#include <conexion.h>

int iniciar_servidor(char *puerto) {
    int socket_servidor;

    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, puerto, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family,
                             servinfo->ai_socktype,
                             servinfo->ai_protocol);

    bool bind_correcto = !(bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen));

    listen(socket_servidor, SOMAXCONN);
    freeaddrinfo(servinfo);

    return bind_correcto ? socket_servidor : -1;
}

int esperar_cliente(int socket_servidor) {
    return accept(socket_servidor, NULL, NULL);
}

int conectarse_a_servidor(char *ip, char *puerto) {
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    bool conexion_correcta = !(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen));

    freeaddrinfo(server_info);

    return conexion_correcta ? socket_cliente : -1;
}

void liberar_conexion(int socket_cliente) {
    close(socket_cliente);
}
