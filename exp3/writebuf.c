#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <error.h>

#define SHMKEY 100
#define SEMKEY 6667
#define SIZE 10 //缓冲区个数

/*
	缓冲区结构
	val: 数据
	num: 数据个数
*/
typedef struct Item{
	char val[1024];
	int num;
}Item;

union semun{
	int val;
	struct semid_ds* buf;
	ushort* array;
	struct seminfo* __buf;
	void *_pad;
};

void P(int semid, int index);
void V(int semid, int index);
void my_write();

int sem_id = 0;
int shm_id = 0;
int out = 0, in = 0;
FILE *f_in = NULL;

int main(int argc, char** argv){
	if ((f_in = fopen(argv[1], "rb")) == NULL){
		perror("fopen error");
		return 0;
	}
	/*创建共享内存组*/
	shm_id = shmget(SHMKEY, sizeof(Item)*SIZE, IPC_CREAT|0666);
	if (shm_id == -1){
		perror("shmget error");
		return -1;
	}	
	/*创建信号量*/
	sem_id = semget(SEMKEY, 3, IPC_CREAT|0666);
	if (sem_id == -1){
		perror("semget error");
		return -1;
	}
	my_write();
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

void my_write(){
	int write_num;
	Item *p_map = (Item *)shmat(shm_id, NULL, 0);
	long count = 0;
	while(1){
		P(sem_id, 0);
		P(sem_id, 2);
		write_num = fread((p_map+in)->val, sizeof(char), 1024, f_in);
		//strncpy((p_map+in)->val, temp, sizeof(temp));
		(p_map+in)->num = write_num;
		in = (in + 1) % SIZE;
		V(sem_id, 2);
		V(sem_id, 1);
		count += write_num;
		if(write_num < 1024) 	break;
	}
	printf("read file over: %ld bytes\n", count);
	shmdt(p_map);
	fclose(f_in);
}
