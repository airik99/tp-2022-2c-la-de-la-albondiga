#include <serializacion.h>

t_paquete *crear_paquete(op_code codigo_op)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void crear_buffer(t_paquete *paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void *serializar_paquete(t_paquete *paquete, int bytes)
{
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

void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void eliminar_paquete(t_paquete *paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}


void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2 * sizeof(int);
	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

//-------------------------------------------------------------------------------------------------//

void serializar_instrucciones(t_paquete *paquete, t_list *instrucciones, char **segmentos)
{
	int cantidad_instrucciones = list_size(instrucciones);

	agregar_a_paquete(paquete, &cantidad_instrucciones, sizeof(int));
	for (int i = 0; i < cantidad_instrucciones; i++)
	{
		t_instruccion *instruccion = list_get(instrucciones, i);
		int largo_instruccion = strlen(instruccion->nombre) + 1;

		agregar_a_paquete(paquete, &largo_instruccion, sizeof(int));
		agregar_a_paquete(paquete, instruccion->nombre, largo_instruccion);

		int cantidad_parametros = list_size(instruccion->params);
		agregar_a_paquete(paquete, &cantidad_parametros, sizeof(int));
		for (int j = 0; j < cantidad_parametros; i++)
		{
			char *parametro = list_get(instruccion->params, j);
			int largo_parametro = (strlen(parametro)) + 1;
			agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
			agregar_a_paquete(paquete, parametro, largo_parametro);
		}
	}

	for (int i = 0; segmentos[i] != NULL; i++)
	{
		int *valor = malloc(sizeof(int));	  
		int temp_val = atoi(segmentos[i]);				  
		memcpy(valor, &temp_val, sizeof(int)); 
		agregar_a_paquete(paquete, valor, sizeof(int));
	}
}
/*
void recibir_instrucciones(t_paquete *paquete, t_list *instrucciones, char **segmentos, int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		int cantidad_instrucciones;
		memcpy(&tamanio, buffer, sizeof(int));
		agregar_a_paquete(paquete, &cantidad_instrucciones, sizeof(int));
		for (int i = 0; i < cantidad_instrucciones; i++)
		{
			t_instruccion *instruccion = list_get(instrucciones, i);
			int largo_instruccion = strlen(instruccion->nombre) + 1;

			agregar_a_paquete(paquete, &largo_instruccion, sizeof(int));
			agregar_a_paquete(paquete, instruccion->nombre, largo_instruccion);

			int cantidad_parametros = list_size(instruccion->params);
			agregar_a_paquete(paquete, &cantidad_parametros, sizeof(int));
			for (int j = 0; j < cantidad_parametros; i++)
			{
				char *parametro = list_get(instruccion->params, j);
				int largo_parametro = (strlen(parametro)) + 1;
				agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
				agregar_a_paquete(paquete, parametro, largo_parametro);
			}
		}
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}*/