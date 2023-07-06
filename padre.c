//este archivo es el fichero fuente que al compilarse produce el ejecutable PADRE
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

/*Prototipo función para inicializar semáforo (pag 304 del libro base)*/
int init_sem (int semid, ushort valor);

/*Prototipo función para señal wait_sem (pag 305 del libro base)*/
int wait_sem (int semid);

/*Prototipo función para señal signal_sem (pag 305 del libro base)*/
int signal_sem (int semid);

void main(int argc, char *argv[])
{

//Tamaño máximo de zona de memoria compartida
const size_t TAM_MAX = 100;

// Variables auxiliares
int a;
int pid;
char tuberia1[12], tuberia2[12], numer[12];
char *generador,*numero,*tub1,*tub2,*proc;
int escritura;

//Contador de procesos vivos
int K;

//Contador de ronda
int contador_rond;

//Longitud de los mensajes enviados o recibidos por la cola de mensajes
int longitud;

// Estructura que define los mensajes que leemos en la cola de mensajes
struct mensaje_c {
	long tipo;
	int pid;
	char estado[2];
	};

// Numero de procesos a crear, es parámetro del ejecutable
int num_proc = atoi(argv[1]);

//Creamos clave asociada al nombre del ejecutable y la letra X
key_t clave = ftok(argv[0],'X');

//Mensaje a mandar por la tuberia barrera
char mensaje;

//Creamos cola de mensajes a partir de la clave
int mensajes = msgget(clave,0777|IPC_CREAT);
//Comprobamos si se ha creado correctamente
if (mensajes==-1)
{
	perror("msgget");
	exit(2);
}

//Creamos región de memoria compartida
int lista = shmget(clave,TAM_MAX,0777|IPC_CREAT);
//Comprobamos si se ha creado correctamente
if (lista==-1)
{
	perror("shmget:");
	exit(2);
}
// Array con los PIDS de los procesos creados
int *Array_PIDS;
//Enlazamos array a la región de memoria compartida
Array_PIDS = shmat(lista,0,0);
//Comprobamos que la operación se realiza de manera correcta
if ((int)Array_PIDS==-1)
{
	perror("shmat:");
	exit(2);
}

//Creamos semaforo para controlar acceso a variable compartida
int sem = semget(clave,1,0777|IPC_CREAT);

//Comprobamos que el semáforo se crea correctamente
if (sem==-1)
{
	perror("sem:");
	exit(2);
}
//Lo inicializamos a 1
init_sem(sem,1);

//Creamos tubería barrera y comprobamos si se crea correctamente
int barrera[2];
if (pipe(barrera)==-1){ perror("pipe"); exit(2); };

// Creamos N procesos hijos y les pasamos como argumentos generador de clave de padre, numero de hijo, tuberia y número de procesos creados
generador=argv[0];
sprintf(tuberia1, "%d", barrera[0]);
tub1=tuberia1;
sprintf(tuberia2, "%d", barrera[1]);
tub2=tuberia2;
proc=argv[1];
for (a = 0;a < num_proc; a = a+1){
	if ((pid=fork())==-1)
		{ perror("fork"); exit(2); }
	else if (pid==0){
		//Pasamos numero de hijo a char
		sprintf(numer, "%d", a);
                numero=numer;
		//Proceso hijo que ejecuta el ejecutable hijo
		execl("./Trabajo2/HIJO",generador,numero,tub1,tub2,proc,NULL);
		}	
	}

//Esperamos a que todos los hijos sean creados y los PIDS almacenados
usleep(10000);

//Inicializamos contador de procesos vivos
K = num_proc;

//Inicializamos contador de ronda
contador_rond = 0;

//Iniciamos ronda tras comprobar los procesos que queden vivos
while (K>1){
contador_rond=contador_rond+1;
printf("\n");
printf("INICIANDO RONDA DE ATAQUES %d \n",contador_rond);
printf("Quedan %d hijos vivos\n",K);
printf("\n");
fflush(stdout);

//Mandamos un byte por cada hijo vivo para que comience la ronda
mensaje = '1';
for(a = 0; a < K; a = a+1){
      escritura = write(barrera[1],&mensaje, 1);
      if (escritura < 0)
    	{
        perror("write");
	exit(2);
        break;
	}
	}
//Esperamos a que termine la ronda
usleep(300000);

//Añadimos mensaje para identificar final de la cola de mensajes
struct mensaje_c mensaje_final;
mensaje_final.pid = 0;
mensaje_final.tipo=2;
strcpy(mensaje_final.estado, "FI");
longitud=sizeof(mensaje_final)-sizeof(mensaje_final.tipo);
if(msgsnd(mensajes,&mensaje_final,longitud,0)==-1)
	{
	perror("msgsnd");
	exit(2);
	}

//Leemos los mensajes de la cola de mensajes hasta detectar el mensaje final
struct mensaje_c mensaje_cola;
while (true){
	if(msgrcv(mensajes, &mensaje_cola,longitud,2,0)==-1)
		{
		perror("msgrcv");
		exit(2);
		}
	if (mensaje_cola.estado[0]=='F'){break;}
	//Terminamos procesos con estado "KO", actualizamos numero de procesos activos, ponemos a 0 su PID en la lista
	if (mensaje_cola.estado[0]=='K')
		{
		printf("Padre mata a proceso %d \n",mensaje_cola.pid);
		fflush(stdout);
		kill(mensaje_cola.pid,SIGKILL);
		//Esperamos a que el hijo termine
		waitpid(mensaje_cola.pid,0,0);
		wait_sem(sem);
		for (a=0;a<num_proc;a=a+1)
			{
			if (Array_PIDS[a] == mensaje_cola.pid){Array_PIDS[a]=0;}
			}
		signal_sem(sem);
		K=K-1;
		}
	}

//Si solo queda un hijo lo declaramos ganador
if (K==1) 
	{
	wait_sem(sem);
	for (a=0;a<num_proc;a=a+1)
		{
		if (Array_PIDS[a] != 0)
		{
			printf("\n");
        		printf("--LA PARTIDA HA TERMINADO--\n");
        		printf("Resultado:\n");
			printf("El hijo %d ha ganado. \n",Array_PIDS[a]);fflush(stdout);}
		}
	signal_sem(sem);
	}
//Si no quedan hijos vivos anunciamos un empate
if (K==0)
	{
	printf("\n");	
	printf("--LA PARTIDA HA TERMINADO--\n");
	printf("Resultado:\n");	
	printf("Empate\n");
	fflush(stdout);
	}
}

//Borramos cola de mensajes
msgctl(mensajes, IPC_RMID,0);

//Borramos semaforo
semctl(sem, IPC_RMID,0);

//Cerramos tuberia
close (barrera[0]);
close (barrera[1]);

//Borramos zona de memoria compartida
shmdt(Array_PIDS);
shmctl(lista, IPC_RMID,0);

//Mostramos semáforos y colas de mensajes activos
printf("\n");
printf ("Semaforos y colas de mensajes activos:\n");
system("ipcs -sq");
}


int init_sem (int semid, ushort valor)
{
ushort sem_array[1];
sem_array[0]=valor;
if (semctl(semid,0,SETALL,sem_array)==-1)
	{
		perror("Error semctl");
		return -1;
	}
	return 0;
}

int wait_sem (int semid)
{
struct sembuf op[1];
op[0].sem_num=0;
op[0].sem_op=-1;//Operación P, decremento uno
op[0].sem_flg=0;
if (semop(semid, op, 1) == -1)
	{
	perror("Error semop:");
	return -1;
	}
return 0;
}

int signal_sem (int semid)
{
struct sembuf op[1];
op[0].sem_num=0;
op[0].sem_op=1;//Operación V, sumo uno
op[0].sem_flg=0;
if (semop(semid, op, 1) == -1)
	{
	perror("Error semop:");
	return -1;
	}
return 0;
}
