#include <consola.h>
int main(int argc, char **argv)
{
	//if(argc < 3) {
	//	return EXIT_FAILURE;
	//}
	//char* =argv[1];
	//char* = argv[2];

	t_config *config = config_create("cfg/Consola.config");
	t_list *instrucciones = leer_archivo("cfg/pseudocodigo.txt");
	t_log *logger = log_create("cfg/Consola.log", "Consola", true, LOG_LEVEL_INFO);

	char *ip = config_get_string_value(config, "IP_KERNEL");
	char *puerto = config_get_string_value(config, "PUERTO_KERNEL");
	 char **segmentos = config_get_array_value(config, "SEGMENTOS");

	log_info(logger, "Consola iniciada. Intentando conectarse al kernel");

	int socket_cliente = conectarse_a_servidor(ip, puerto);

	if (socket_cliente == -1)
	{
		log_info(logger, "Error en la conexion al servidor. Terminando consola");
		return EXIT_FAILURE;
	}

	log_info(logger, "Conexion correcta. Enviando instrucciones(todavia no esta hecho)");
	t_paquete *paquete_instrucciones = crear_paquete(INSTRUCCIONES);
	list_iterate(instrucciones, (void *)print_instruccion);

	/*recv(socket_cliente,&respuesta,sizeof(uint32_t),MSG_WAITALL);
		if(respuesta == 0)log_info(logger, "Finalizacion exitosa");
		else			  log_error(logger, "Ocurrio un error con la ejecucion del proceso");
	*/


	liberar_conexion(socket_cliente);
	string_array_destroy(segmentos);
	destructor_instrucciones(instrucciones);
	eliminar_paquete(paquete_instrucciones);
	config_destroy(config);
	log_destroy(logger);
	return EXIT_SUCCESS;
}

t_list *leer_archivo(char *nombre)
{
	FILE *puntero;
	char *linea_leida;
	puntero = fopen(nombre, "r");
	t_list *lista_instrucciones = list_create();

	if (!puntero)
	{
		printf("No se pudo leer el archivo \n");
		exit(1);
	}

	while (!feof(puntero))
	{
		linea_leida = leer_linea(puntero);
		char **valores = string_split(linea_leida, " ");
		char *nombre = strdup(valores[0]);
		t_list *params = list_create();
		for (int i = 1; valores[i] != NULL; i++)
		{
			char *param = strdup(valores[i]);
			list_add(params, param);
		}
		t_instruccion *instruccion = crear_instruccion(nombre, params);
		list_add(lista_instrucciones, instruccion);
		free(linea_leida);
		string_array_destroy(valores);
	}
	txt_close_file(puntero);
	return lista_instrucciones;
}

//Lee una linea del archivo y la retorna como char*
char *leer_linea(FILE *puntero)
{

	ssize_t bytes_leidos;
	size_t numero_bytes;
	char *cadena;

	numero_bytes = 0;
	cadena = NULL;
	bytes_leidos = getline(&cadena, &numero_bytes, puntero);
	if (bytes_leidos == -1)
	{
		puts("Error al leer la linea del archivo");
	}
	//saca el newline a las cadenas que lo tengan
	if (string_ends_with(cadena, "\n"))
	{
		cadena[strlen(cadena) - 1] = '\0';
	}
	return cadena;
}

t_instruccion *crear_instruccion(char *nombre, t_list *params)
{
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	instruccion->nombre = nombre;
	instruccion->params = params;
	return instruccion;
}
