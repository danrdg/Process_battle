//este archivo es el fichero fuente que al compilarse produce el ejecutable HIJO
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

/*Prototipo función para señal wait_sem (pag 305 del libro base)*/
int wait_sem (int semid);

/*Prototipo función para señal signal_sem (pag 305 del libro base)*/
int signal_sem (int semid);

/*Prototipo funcion defensa*/
void defensa();

/*Prototipo funcion indefenso*/
void indefenso();

//Variable global de estado del proceso hijo
char estado[2];

void main(int argc, char *argv[]){

//Tamaño máximo de zona de memoria compartida
const size_t TAM_MAX = 100;

//Array con los pids de los procesos hijos
int Array_PIDS[100];

//Variables auxiliares 
int a,lectura;
bool continuar;

//Mensaje a recibir por la tuberia barrera
char mensaje;

//Longitud del mensaje a enviar por la cola de mensajes
int longitud;

// Estructura que define los mensajes que pasamos en la cola de mensajes
struct mensaje_c {
	long tipo;
	int pid;
	char estado[2];
	};
struct mensaje_c mensaje_cola;

//Semilla para generar números aleatorios y números a generar
int semilla=getpid();
int aleatorio, aleatorio2;

//Almacenamos numero de procesos creados
int num_proc = atoi(argv[4]);

//Volvemos a crear la clave
key_t clave = ftok(argv[0],'X');

//Volvemos a crear cola de mensajes
int mensajes = msgget(clave,0777|IPC_CREAT);

//Volvemos a crear región de memoria compartida
// Puntero a Array con los PIDS de los procesos creados
int *Array_lista;
int lista = shmget(clave,TAM_MAX,0777|IPC_CREAT);
Array_lista = shmat(lista,0,0);

//Volvemos a crear semaforo para controlar acceso a variable compartida
int sem = semget(clave,1,0777|IPC_CREAT);

//Comprobamos que el semáforo se crea correctamente
if (sem==-1)
{
	perror("sem:");
	exit(2);
}

//Volvemos a crear tuberia barrera
int barrera[2];
barrera[0] = atoi(argv[2]);
barrera[1] = atoi(argv[3]);

//Asignamos PID a array en zona de memoria compartida
int num = atoi (argv[1]);

//Actualizamos Array con PID propio controlando el acceso al recurso compartido con el semáforo

wait_sem(sem);
Array_lista[num] = getpid();
signal_sem(sem);

//Leemos el byte de PADRE por la tubería barrera para comenzar la ronda
while(true){

lectura = read(barrera[0],&mensaje, 1);
if (lectura < 0)
{
        perror("read");
}

//Leemos Array de memoria compartida
wait_sem(sem);
for (a=0;a<num_proc;a=a+1)
{
	Array_PIDS[a]=Array_lista[a];
}
signal_sem(sem);
//Actualizamos variable de estado
estado[0]='O';
estado[1]='K';

//Generamos numero aleatorio entre 1 y 2 para determinar estrategia a seguir
semilla = semilla+1;
srand (semilla);
aleatorio = (rand() % 2);

//Si el numero aleatorio es 0 instalamos la funcion defensa en SIGURS1
if (aleatorio==0){
	if (signal(SIGUSR1,defensa)==SIG_ERR)
		{
		perror("signal:");
		exit(2);
		}
	//Y duerme 0.2 segundos
	usleep(200000);
}

//Si el numero aleatorio es 1 instalamos la funcion indefenso en SIGURS1
if (aleatorio==1){
	if (signal(SIGUSR1,indefenso)==SIG_ERR)
		{
		perror("signal:");
		exit(2);
		}
	//Y duerme 0.1 segundos
	usleep(100000);
        //Para continuar con la fase de ataque, eligiendo un proceso aleatorio entre los creados
	continuar = true;
	while (continuar)
		{
		aleatorio2 = (rand()%(num_proc));
		//Comprobamos si el proceso elegido sigue con vida
		//Si sigue con vida y no es su propio PID dejamos de buscar 
		if ((Array_PIDS[aleatorio2]!=0)&&(Array_PIDS[aleatorio2]!=getpid()))
			{
			//Imprimimos mensaje y mandamos señal SIGUSR1 al proceso atacado de manera aleatoria
			printf("Proceso %d atacando al proceso %d\n",getpid(),Array_PIDS[aleatorio2]);
			fflush(stdout);
			kill (Array_PIDS[aleatorio2],SIGUSR1);
			continuar=false;
			}
		semilla = semilla+1;
		srand(semilla);
		}
	}
// Y esperamos otros 0.1 segundos a que termine la ronda de ataques
usleep(100000);

//Terminada la ronda de ataques y defensas, informamos a Padre por la cola de mensajes
mensaje_cola.pid=getpid();
if (estado[0]=='O'){strcpy(mensaje_cola.estado, "OK");}
if (estado[0]=='K'){strcpy(mensaje_cola.estado, "KO");}
mensaje_cola.tipo=2;
longitud=sizeof(mensaje_cola)-sizeof(mensaje_cola.tipo);
if(msgsnd(mensajes,&mensaje_cola,longitud,0)==-1)
	{
	perror("msgsnd");
	exit(2);
	}
}


}
/*Funcion defensa*/
void defensa()
{
printf("El hijo %d ha repelido un ataque\n",getpid());
fflush(stdout);
estado[0]='O';
estado[1]='K';
}

/*Funcion indefenso*/
void indefenso()
{
printf("El hijo %d ha sido emboscado mientras realizaba un ataque\n",getpid());
fflush(stdout);
estado[0]='K';
estado[1]='O';
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
