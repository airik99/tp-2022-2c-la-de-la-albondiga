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

void enviar_mensaje(char* mensaje, int socket_cliente) {
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

//-------------------------------------------------------------------------------------------------//
void serializar_instrucciones(t_paquete *paquete, t_list *instrucciones /*,char **segmentos*/)
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
		for (int j = 0; j < cantidad_parametros; j++)
		{
			char *parametro = list_get(instruccion->params, j);
			int largo_parametro = strlen(parametro) + 1;
			agregar_a_paquete(paquete, &largo_parametro, sizeof(int));
			agregar_a_paquete(paquete, parametro, largo_parametro);
		}
	}

	/* Descomentar cuando tenga bien resuelta la deserializacion de los segmentos
	for (int i = 0; segmentos[i] != NULL; i++)
	{
		int *valor = malloc(sizeof(int));
		int temp_val = atoi(segmentos[i]);
		memcpy(valor, &temp_val, sizeof(int));
		agregar_a_paquete(paquete, valor, sizeof(int));
	}*/
}

t_instruccion *deserializar_instruccion(void *buffer, int *desplazamiento)
{
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	instruccion->params = list_create();
	int largo_nombre, cant_params;

	memcpy(&largo_nombre, buffer + *desplazamiento, sizeof(int));
	*desplazamiento += sizeof(int);

	instruccion->nombre = malloc(largo_nombre);
	memcpy(instruccion->nombre, buffer + *desplazamiento, largo_nombre);
	*desplazamiento += largo_nombre;

	memcpy(&cant_params, buffer + *desplazamiento, sizeof(int));
	*desplazamiento += sizeof(int);

	for (int i = 0; i < cant_params; i++)
	{
		int largo_param;
		memcpy(&largo_param, buffer + *desplazamiento, sizeof(int));
		char *param = malloc(largo_param);
		list_add(instruccion->params, param);
		*desplazamiento += sizeof(int);
	}
	return instruccion;
}

t_list* recibir_instrucciones(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	int cantidad_instruccciones;
	void *buffer;

	t_list* instrucciones = list_create();
	buffer = recibir_buffer(&size, socket_cliente);

	memcpy(&cantidad_instruccciones, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	for(int i = 0; i < cantidad_instruccciones; i++)
	{
		t_instruccion *instruccion = deserializar_instruccion(buffer, &desplazamiento);
		list_add(instrucciones, instruccion);
	}
	/*while(desplazamiento < size)
	{
		//Aca va lo de los segmentos despues tengo que ver bien como recibirlo
	}*/
	free(buffer);
	return instrucciones;
}