#ifndef ESTRUCTURASCOMUNES_H_
#define ESTRUCTURASCOMUNES_H_

#include <sys/types.h>
#include <commons/collections/list.h>
#define MAX_CLIENTS 30

enum procesos {
	kernel, cpu, consola, file_system, memoria
};

enum accionesFS{
	k_fs_validar_archivo, k_fs_crear_archivo, k_fs_borrar_archivo, k_fs_leer_archivo, k_fs_escribir_archivo
};

enum accionesMemoria {
	asignarPaginas, finalizarProceso
};

enum accionesCPU{
	cpuLibre, cpu_k_abrir_archivo, cpu_k_cerrar_archivo, cpu_k_borrar_archivo, cpu_k_mover_cursor_archivo,
	cpu_k_leer_archivo, cpu_k_escribir_archivo
};

enum confirmacion {
	noHayPaginas, hayPaginas
};

enum acciones {
	startProgram
};

typedef struct{
	int clie_CPU;
	int libre;
} cliente_CPU;

typedef struct {
	int PUERTO_PROG;
	int PUERTO_CPU;
	char IP_MEMORIA[15];
	int PUERTO_MEMORIA;
	char IP_FS[15];
	int PUERTO_FS;
	int QUANTUM;
	int QUANTUM_SLEEP;
	char ALGORITMO[30];
	int GRADO_MULTIPROG;
	char SEM_IDS[10][30]; // Debería ser una lista alfanumerica
	int SEM_INIT[10][30]; // Lo mismo pero numerica
	char SHARED_VARS[10][30]; // IDEM SEM_IDS
	int STACK_SIZE;
} t_configuracion;

t_configuracion *config;

t_list *listaPCBs_NEW;
t_list *listaPCBs_READY;
t_list *listaPCBs_EXEC;
t_list *listaPCBs_BLOCK;
t_list *listaPCBs_EXIT;
t_list *listaCPUs;

int cliente, cliente2, servMemoria, servFS;
u_int32_t tamanioPagMemoria;

#endif /* ESTRUCTURASCOMUNES_H_ */
