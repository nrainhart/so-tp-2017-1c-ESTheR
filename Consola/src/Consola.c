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
#include "libreriaSockets.h"

#define RUTAARCHIVO "/home/utnso/git/tp-2017-1c-C-digo-Facilito/Consola/src/ConfigConsola.txt "

int horaInicio;
int minInicio;
int segInicio;
int horaFin;
int minFin;
int segFin;

typedef struct {
	char* ipKernel;
	int puerto;
} t_configuracion;
t_configuracion *config;

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum acciones {
	startProgram, endProgram
};

enum confirmacionMem {
	noHayPaginas, hayPaginas
};

void llenarSocket(struct sockaddr_in *direccionServidor) {
	(*direccionServidor).sin_family = AF_INET;
	(*direccionServidor).sin_addr.s_addr = inet_addr(config->ipKernel);
	(*direccionServidor).sin_port = htons(config->puerto);
}

void informarAccion(int *cliente, int *accion) {
	u_int32_t acc = (*accion);
	send((*cliente), &acc, sizeof(u_int32_t), 0);
}

void solicitarA(int *cliente, char *nombreCli) {
	char a[2] = "a";
	send((*cliente), a, 2, 0);
	printf("Esperando atencion de %s..\n", nombreCli);
	recv((*cliente), a, 2, 0);
}

void msjConexionCon(char *s) {
	printf("\n-------------------------------------------\nEstoy conectado con %s\n-------------------------------------------\n",s);
} //Despues la borramos, la dejo para que tire el mensaje de con quien se conecta en el handshake.

void *reservarMemoria(int tamanioArchivo) {
	void *puntero = malloc(tamanioArchivo);
	if (puntero == NULL) {
		printf("No hay más espacio \n");
		exit(-1);
	}
	return puntero;
}

void settearVariables(t_config *archivo_Modelo) {
	config = reservarMemoria(sizeof(t_configuracion));
	config->ipKernel = strdup(
			config_get_string_value(archivo_Modelo, "IP_KERNEL"));
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
		printf("No se encontró el Archivo \n");
		exit(-1);
	}
	t_config *archivo_config = config_create(RUTAARCHIVO);
	settearVariables(archivo_config);
	config_destroy(archivo_config);
	//mostrarArchivoConfig();
	printf("Leí el archivo y extraje el puerto: %d \n\n", config->puerto);
}

void mostrarConfirmacion(int confirmacion) {
	u_int32_t conf = confirmacion;
	if (conf == hayPaginas) {
		printf("Paginas suficientes - El proceso se almaceno exitosamente.\n");
	} else {
		printf(
				"Paginas insuficientes - El proceso no pudo almacenarse en MP.\n");
	}
}

void esperarConfirmacionDeKernel(int *kernel) {
	u_int32_t confirmacion;
	printf("Esperando la confirmacion de Kernel..\n");
	if (recv((*kernel), &confirmacion, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo la confirmacion de parte de Kernel.\n");
		exit(-1);
	}
	mostrarConfirmacion(confirmacion);
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

	printf("Hora: %d\n", difHoras);
	printf("Minuto: %d\n", difMinutos);
	printf("Segundo: %d\n", difSegundos);
}

void mostrarFechaHoraEjecucion(int opcion) {
	struct tm *tmRetorno;
	time_t tiempoEnSegundos = time(NULL);
	struct tm *tm = localtime(&tiempoEnSegundos);
	tmRetorno = tm;
	printf("Fecha: %s\n", asctime(tmRetorno));
	printf("Hora: %d\n", tmRetorno->tm_hour);
	printf("Minuto: %d\n", tmRetorno->tm_min);
	printf("Segundo: %d\n", tmRetorno->tm_sec);

	if (opcion == 1) {
		horaInicio = tmRetorno->tm_hour;
		minInicio = tmRetorno->tm_min;
		segInicio = tmRetorno->tm_sec;
	} else {
		horaFin = tmRetorno->tm_hour;
		minFin = tmRetorno->tm_min;
		segFin = tmRetorno->tm_sec;
	}
}

void mostrarInicioEjecucion() {
	printf("Fecha y hora de inicio de ejecucion:\n");
	mostrarFechaHoraEjecucion(1);
}

void mostrarFinEjecucion() {
	printf("Fecha y hora de fin de ejecucion:\n");
	mostrarFechaHoraEjecucion(2);
}

void esperarMensajesDeKernel() {
	//printf("Esperando mensajes de Kernel...\n");
}

void crearHiloDelPrograma() {
	pthread_t hilo_programa;
	printf("Creado hilo para este programa.\n");
	printf("Se mostrarán por pantalla las respuestas del Kernel.\n\n");
	if (pthread_create(&hilo_programa, NULL, esperarMensajesDeKernel, NULL)) {
		printf("Error al crear el thread de comandos.\n");
		exit(-1);
	}
}

void recibirPID(int *cliente) {
	u_int32_t pid;
	if (recv((*cliente), &pid, sizeof(u_int32_t), 0) == -1) {
		printf("Error recibiendo el PID\n");
		exit(-1);
	}
	printf("PID: %d asignado a ese programa\n\n", pid);
}

void iniciarPrograma(int *cliente) {
	int accion;
	/*Iniciar Programa: Este comando iniciará un nuevo Programa AnSISOP, recibiendo por
	 parámetro el path del script AnSISOP a ejecutar. Una vez iniciado el programa la consola
	 quedará a la espera de nuevos comandos, pudiendo ser el iniciar nuevos Programas AnSISOP
	 o algunas de las siguientes opciones. Quedará a decisión del grupo utilizar paths absolutos o
	 relativos y deberán fundamentar su elección.*/

	char *lineaIngresada, *comando, *nombreScript;
	FILE *archivo;

	printf("\nIngrese iniciarPrograma + nombre del script AnSISOP. \n");
	printf("Ejemplo: iniciarPrograma script.ansisop\n\n");

	lineaIngresada = reservarMemoria(100);

	fgets(lineaIngresada, 100, stdin);

	comando = strtok(lineaIngresada, " ");
	nombreScript = strtok(NULL, "\n");

	if (strcmp("iniciarPrograma", comando) != 0) {
		printf("El comando ingresado no existe");
	} else {
		archivo = fopen(nombreScript, "rb"); //USAR PATH ABSOLUTO?
		if (archivo == NULL) {
			printf("No se pudo leer el archivo\n");
			exit(-1);
		}
		solicitarA(cliente, "Kernel");
		accion = startProgram;
		informarAccion(cliente, &accion);

		mostrarInicioEjecucion();
	}

	fseek(archivo, 0, SEEK_END);
	u_int32_t fsize = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);

	char *buffer = reservarMemoria(fsize + 1);
	fread(buffer, fsize, 1, archivo);
	fclose(archivo);
	buffer[fsize] = '\0';
	if (send(*cliente, &fsize, sizeof(u_int32_t), 0) == -1) {
		printf("Error enviando longitud del archivo\n");
		exit(-1);
	}
	if (send(*cliente, buffer, fsize + 1, 0) == -1) {
		printf("Error enviando archivo\n");
		exit(-1);
	}
	printf("El archivo se envió correctamente\n\n");

	esperarConfirmacionDeKernel(cliente);
	recibirPID(cliente);
	crearHiloDelPrograma();

	free(lineaIngresada);
	free(buffer);
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
	printf("\nConsola desconectada. \n\n");
	exit(-1);
}

void limpiarMensajes() {
	system("clear");
	printf("Consola limpiada! \n\n");
}

void elegirComando(int *cliente) {
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
			iniciarPrograma(cliente);
			break;
		case '2':
			desconectarConsola();
			break;
		case '3':
			finalizarPrograma(cliente);
			break;
		case '4':
			limpiarMensajes();
			break;
		default:
			printf("\nOpcion invalida. Vuelva a elegir una opcion \n\n");
			break;
		}

		free(opcionIngresada);
	} while (seguirAbierto);
}

void conectarseConKernel(int *cliente, struct sockaddr_in *direccionServidor) {
	conectar(cliente, direccionServidor);
	int procesoConectado = handshake(cliente, consola);

	switch (procesoConectado) {
		case kernel:
		msjConexionCon("Kernel");
		elegirComando(cliente);
		break;

		default:
		printf("No me puedo conectar con vos.\n");
		break;
	}

	close(*cliente);
}

int main(void) {
	int cliente;
	struct sockaddr_in direccionServidor;

	leerArchivo();
	llenarSocket(&direccionServidor);
	conectarseConKernel(&cliente, &direccionServidor);

	return 0;
}
