#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define READKEY  6667

void P(int semid, int index);
void V(int semid, int index);
void *subp1(void *arg);
void *subp2(void *arg);
pthread_t p1, p2;
int semid;
int data;

union semun{
	int val;
	struct semid_ds* buf;
	ushort* array;
	struct seminfo* __buf;
	void *_pad;
};

int main(){
	data = 0;
	semid = semget(READKEY, 2, IPC_CREAT|0666);
	if (semid == -1){
		perror("semget");
		exit(EXIT_FAILURE);
	}
	union semun sem_args;
	sem_args.val = 1;
	int ret = semctl(semid, 0, SETVAL, sem_args);
	if (ret == -1){
		perror("semctl");
		exit(EXIT_FAILURE);	
	}
	sem_args.val = 0;
	ret = semctl(semid, 1, SETVAL, sem_args);
	if (ret == -1){
		perror("semctl");
		exit(EXIT_FAILURE);	
	}
	int err = pthread_create(&p1, NULL, subp1, NULL);
	if (err != 0){
		printf("create new thread error : %d \n", err);
		exit(EXIT_FAILURE);
	}
	err = pthread_create(&p2, NULL, subp2, NULL);
	if (err != 0){
		printf("create new thread error : %d \n", err);
		exit(EXIT_FAILURE);
	}
	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	ret = semctl(semid, 0, IPC_RMID, sem_args);
	if (ret == -1){
		perror("semctl");
		exit(EXIT_FAILURE);	
	}
}

void *subp1(void *arg){
	int i = 1;
	for (; i < 100; i++){
		P(semid, 0);
		printf("thread1 : change data from %d to", data);
		data += i;
		printf(" %d \n", data);
		V(semid, 1);
	}
	return NULL;
}

void *subp2(void *arg){
	int i = 1;
	for (; i < 100; i++){
		P(semid, 1);
		printf("thread2 : data is : %d \n", data);
		V(semid, 0);
	}
	return NULL;
}

void P(int semid, int index){
	struct sembuf sem;
	sem.sem_num = index;
	sem.sem_op = -1;
	sem.sem_flg = 0;
	semop(semid, &sem, 1);
	return;
}

void V(int semid, int index){
	struct sembuf sem;
	sem.sem_num = index;
	sem.sem_op = 1;
	sem.sem_flg = 0;
	semop(semid, &sem, 1);
	return;
}
