#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/config.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "

struct tm *tmInicio;
int horaInicio;
int minInicio;
int segInicio;
struct tm *tmFin;
int horaFin;
int minFin;
int segFin;

t_list *lista_hilos_por_PID;

struct sockaddr_in direccionServidor;

int serv_kernel;

typedef struct {
	char* ipKernel;
	int puerto;
} t_configuracion;
t_configuracion *config;

typedef struct{
	pthread_t *hilo;
	int PID;
	char *nombre_script;
} hilo_por_programa;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum acciones {
	startProgram, endProgram
};

enum confirmacionMem {
	noHayPaginas, hayPaginas
};

void llenarSocket() {
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(config->ipKernel);
	direccionServidor.sin_port = htons(config->puerto);
}

void informarAccion(int *cliente, int *accion) {
	u_int32_t acc = (*accion);
	send((*cliente), &acc, sizeof(u_int32_t), 0);
}

void solicitarA(int *cliente, char *nombreCli) {
	char a[2] = "a";
	send((*cliente), a, 2, 0);
	log_info(consola_log, "Esperando atencion de %s..\n", nombreCli);
	//printf("Esperando atencion de %s..\n", nombreCli);
	recv((*cliente), a, 2, 0);
}

void msjConexionCon(char *s) {
	log_info(consola_log, "\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n", s);
	//printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
	//Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.
}

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		log_error(consola_log, "No hay más espacio");
		//printf("No hay más espacio \n");
		exit(-1);
	}
	return puntero;
}

hilo_por_programa *crear_hilo_por_programa(){
	hilo_por_programa *hilo_por_PID = reservarMemoria(sizeof(hilo_por_programa));
	return hilo_por_PID;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->ipKernel = strdup(config_get_string_value(archivo_Modelo, "IP_KERNEL"));
	config->puerto = config_get_int_value(archivo_Modelo, "PUERTO_KERNEL");
}
void mostrarArchivoConfig() {
	FILE *f;

	f = fopen(RUTAARCHIVO, "r");
	int c;
	printf("------------------------------------------\n");
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	printf("\n");
	printf("------------------------------------------\n");

}

void leerArchivo() {
	if (access(RUTAARCHIVO, F_OK) == -1) {
		log_error(consola_log, "No se encontró el Archivo");
		//printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	//mostrarArchivoConfig();
	log_info(consola_log, "Leí el archivo y extraje el puerto: %d ", config->puerto);
	//printf("Leí el archivo y extraje el puerto: %d \n\n", config->puerto);
}

void mostrarConfirmacion(int confirmacion) {
	u_int32_t conf = confirmacion;
	if (conf == hayPaginas) {
		log_info(consola_log, "Paginas suficientes - El proceso se almaceno exitosamente");
		//printf("Paginas suficientes - El proceso se almaceno exitosamente.\n");
	} else {
		log_error(consola_log, "Paginas insuficientes - El proceso no pudo almacenarse en MP");
		//printf("Paginas insuficientes - El proceso no pudo almacenarse en MP.\n");
	}
}

void esperarConfirmacionDeKernel(int *kernel) {
	u_int32_t confirmacion;
	log_info(consola_log, "Esperando la confirmacion de Kernel..");
	//printf("Esperando la confirmacion de Kernel..\n");
	if (recv((*kernel), &confirmacion, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error recibiendo la confirmacion de parte de Kernel");
		//printf("Error recibiendo la confirmacion de parte de Kernel.\n");
		exit(-1);
	}
	mostrarConfirmacion(confirmacion);
}

void mostrarInicioEjecucion(){
	log_info(consola_log,"Inicio Ejecucion: %ld",tmInicio);
}

void mostrarFinEjecucion(){
	log_info(consola_log,"Fin Ejecucion: %ld",tmFin);
}

void mostrarDiferenciaInicioFinEjecucion() {
	int difHoras, difMinutos, difSegundos;

	if (segInicio > segFin) {
		--minFin;
		segFin += 60;
	}

	difSegundos = segFin - segInicio;
	if (minInicio > minFin) {
		--horaFin;
		minInicio += 60;
	}

	difMinutos = minFin - minInicio;
	difHoras = horaFin - horaInicio;

	log_info(consola_log, "Hora: %d", difHoras);
	//printf("Hora: %d\n", difHoras);
	log_info(consola_log, "Minuto: %d", difMinutos);
	//printf("Minuto: %d\n", difMinutos);
	log_info(consola_log, "Segundo: %d", difSegundos);
	//printf("Segundo: %d\n", difSegundos);
	//Revisar si vale la pena usar logs
}

void mostrarFechaHoraEjecucion(){
	mostrarInicioEjecucion();
	mostrarFinEjecucion();
	mostrarDiferenciaInicioFinEjecucion();
}

void guardarFechaHoraEjecucion(int opcion) {
	time_t tiempoEnSegundos = time(NULL);

	if (opcion == 1) {
		tmInicio = localtime(&tiempoEnSegundos);
		horaInicio = tmInicio->tm_hour;
		minInicio = tmInicio->tm_min;
		segInicio = tmInicio->tm_sec;
	} else {
		tmFin = localtime(&tiempoEnSegundos);
		horaFin = tmFin->tm_hour;
		minFin = tmFin->tm_min;
		segFin = tmFin->tm_sec;
	}
}

void guardarInicioEjecucion() {
	guardarFechaHoraEjecucion(1);
}

void guardarFinEjecucion() {
	guardarFechaHoraEjecucion(2);
}

void esperarMensajesDeKernel() {
	//printf("Esperando mensajes de Kernel...\n");
}

void crearHiloDelPrograma() {
	pthread_t hilo_programa;
	log_info(consola_log, "Creado hilo para este programa");
	//printf("Creado hilo para este programa.\n");
	log_info(consola_log, "Se mostrarán por pantalla las respuestas del Kernel");
	//printf("Se mostrarán por pantalla las respuestas del Kernel.\n\n");
	if (pthread_create(&hilo_programa, NULL, esperarMensajesDeKernel, NULL)) {
		log_error(consola_log, "Error al crear el thread de comandos");
		//printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}
}

u_int32_t recibirPID(int *cliente) {
	u_int32_t pid;
	if (recv((*cliente), &pid, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error recibiendo el PID");
		//printf("Error recibiendo el PID\n");
		exit(-1);
	}
	log_info(consola_log, "PID: %d asignado a ese programa\n",pid);
	//printf("PID: %d asignado a ese programa\n\n", pid);
	return pid;
}

void conectarse_con_kernel(){
	conectar(&serv_kernel, &direccionServidor);
	int procesoConectado = handshake(&serv_kernel, consola);
	msjConexionCon("Kernel");
}

char *obtener_un_mensaje(){
	u_int32_t tamanio;
	char *buffer;
	if(recv(serv_kernel,&tamanio,sizeof(u_int32_t),0) == -1){
		printf("Error recibiendo el tamanio del mensaje\n");
	}
	buffer = reservarMemoria(tamanio);
	if(recv(serv_kernel,buffer,tamanio,0) == -1){
		printf("Error recibiendo el mensaje\n");
	}
	return buffer;
}

void recibir_y_mostrar_mensajes(){
	while(1){
		char *mensaje = obtener_un_mensaje();
		printf("%s",mensaje);
		free(mensaje);
	}
}

hilo_por_programa *obtener_hilo_por_programa_segun_hilo(pthread_t *hilo){
	bool es_este_hilo(hilo_por_programa *hiloPorPrograma){
		return *(hiloPorPrograma->hilo) == *hilo;
	}
	return list_find(lista_hilos_por_PID,(void*) es_este_hilo);
}

void hacer_muchas_cosas(pthread_t *hilo){
	int accion;
	FILE *archivo;
	hilo_por_programa *un_hilo_por_programa = obtener_hilo_por_programa_segun_hilo(hilo);
	printf("El nombre del script es: %s\n",un_hilo_por_programa->nombre_script);
	archivo = fopen(un_hilo_por_programa->nombre_script, "rb"); //USAR PATH ABSOLUTO?
	if (archivo == NULL) {
		log_error(consola_log, "No se pudo leer el archivo");
		//printf("No se pudo leer el archivo\n");
		exit(-1);
	}

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	solicitarA(&serv_kernel, "Kernel");
	accion = startProgram;
	informarAccion(&serv_kernel, &accion);

	guardarInicioEjecucion();
	if (send(serv_kernel, &fsize, sizeof(u_int32_t), 0) == -1) {
		log_error(consola_log, "Error enviando longitud del archivo");
		//printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(serv_kernel, buffer, fsize + 1, 0) == -1) {
		log_error(consola_log, "Error enviando archivo");
		//printf("Error enviando archivo\n");
		exit(-1);
	}
	free(buffer);

	log_info(consola_log, "El archivo se envió correctamente\n");
	//printf("El archivo se envió correctamente\n\n");


	esperarConfirmacionDeKernel(&serv_kernel);
	u_int32_t pid = recibirPID(&serv_kernel);

	un_hilo_por_programa->PID = pid;

	recibir_y_mostrar_mensajes();
}

void cormillot(char *lineaIngresada){
	int i = 0;
	while(lineaIngresada[i]!='\n'){
		i++;
	}
	lineaIngresada[i]='\0';
}

void iniciarPrograma() {
	conectarse_con_kernel();
	/*Iniciar Programa: Este comando iniciará un nuevo Programa AnSISOP, recibiendo por
	 parámetro el path del script AnSISOP a ejecutar. Una vez iniciado el programa la consola
	 quedará a la espera de nuevos comandos, pudiendo ser el iniciar nuevos Programas AnSISOP
	 o algunas de las siguientes opciones. Quedará a decisión del grupo utilizar paths absolutos o
	 relativos y deberán fundamentar su elección.*/

	char *lineaIngresada, *comando, *nombreScript;
	nombreScript = reservarMemoria(100);

	lineaIngresada = reservarMemoria(100);

	printf("\nIngrese nombre del script AnSISOP. \n");
	printf("Ejemplo: script.ansisop\n\n");
	fgets(lineaIngresada, 100, stdin);
	cormillot(lineaIngresada);
	strcpy(nombreScript,lineaIngresada);
	free(lineaIngresada);

	hilo_por_programa *un_hilo_por_programa = crear_hilo_por_programa();
	pthread_t *hilo_programa;
	un_hilo_por_programa->hilo = hilo_programa;
	un_hilo_por_programa->nombre_script = nombreScript;
	list_add(lista_hilos_por_PID,un_hilo_por_programa);
	printf("El nombre del script es: %s\n",un_hilo_por_programa->nombre_script);
		if(pthread_create(hilo_programa,NULL,hacer_muchas_cosas,hilo_programa)){
		printf("Error al crear el thread de iniciar programa.\n");
		exit(-1);
	}

}

void finalizarPrograma(int *cliente) {

	int accion;
	char *opcion = reservarMemoria(100);
	int id_proceso_a_detener;

	solicitarA(cliente, "Kernel");
	accion = endProgram;
	informarAccion(cliente, &accion);

	printf("Ingrese el ID del proceso a finalizar: ");
	fgets(opcion, 100, stdin);
	id_proceso_a_detener = atoi(opcion);

	send(*cliente, &id_proceso_a_detener, sizeof(int), 0);

	free(opcion);

	/*Finalizar Programa: Como su nombre lo indica este comando finalizará un Programa
	 AnSISOP, terminando el thread correspondiente al PID que se desee finalizar.*/

}


void desconectarConsola() {
	/*Desconectar Consola: Este comando finalizará la conexión de todos los threads de la consola
	 con el kernel, dando por muertos todos los programas de manera abortiva.*/
	log_info(consola_log, "\nConsola desconectada. \n");
	//printf("\nConsola desconectada. \n\n");
	exit(-1);
}

void limpiarMensajes() {
	system("clear");
	log_info(consola_log,"Consola limpiada! \n");
	//printf("Consola limpiada! \n\n");
}

void hilo_iniciar_programa(){
	pthread_t *hilo_iniciar_programa;
	if(pthread_create(hilo_iniciar_programa,NULL,iniciarPrograma,hilo_iniciar_programa)){
		printf("Error al crear el thread de iniciar programa.\n");
		exit(-1);
	}
	hilo_por_programa *hilo_por_PID = crear_hilo_por_programa();
	hilo_por_PID->hilo = hilo_iniciar_programa;
	list_add(lista_hilos_por_PID,hilo_por_PID);
}

void elegirComando() {
	char *opcionIngresada;
	int seguirAbierto = 1; /*Si se va a cerrar sólo en una de las opciones, tendría que ser
	 directamente la "opcionIngresada" la condición del do-while. Por ahora la dejo así*/

	do {
		printf("Los siguientes comandos estan disponibles para ejecutar:\n");
		printf("1-iniciarPrograma\n");
		printf("2-desconectarConsola\n");
		printf("3-finalizarPrograma\n");
		printf("4-limpiarMensajes\n");

		printf("Ingrese el numero de comando para ejecutarlo:\n");

		opcionIngresada = reservarMemoria(sizeof(opcionIngresada));
		fgets(opcionIngresada, sizeof(opcionIngresada), stdin);

		switch (*opcionIngresada) {
		case '1':
			iniciarPrograma();
			break;
		case '2':
			desconectarConsola();
			break;
		case '3':
			finalizarPrograma(&serv_kernel);
			break;
		case '4':
			limpiarMensajes();
			break;
		default:
			log_error(consola_log, "\nOpcion invalida. Vuelva a elegir una opcion \n");
			//printf("\nOpcion invalida. Vuelva a elegir una opcion \n\n");
			break;
		}

		free(opcionIngresada);
	} while (seguirAbierto);
}

int main(void) {

	lista_hilos_por_PID = list_create();
	inicializarLog();

	leerArchivo();
	llenarSocket();
	elegirComando();

	log_destroy(consola_log);
	return 0;
}
