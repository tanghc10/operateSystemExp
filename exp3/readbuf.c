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
void my_read();

int sem_id = 0;
int shm_id = 0;
int out = 0, in = 0;
FILE *f_out = NULL;

int main(int argc, char** argv){
	if ((f_out = fopen(argv[1], "wb")) == NULL){
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
	my_read();
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

void my_read(){
	//char temp[1024];
	int read_num = 0;
	Item *p_map = (Item *)shmat(shm_id, NULL, 0);
	long count = 0;
	while(1){
		P(sem_id, 1);
		P(sem_id, 2);
		read_num = (p_map+out)->num;
		int num = fwrite((p_map+out)->val, sizeof(char), read_num , f_out);
		out = (out + 1) % SIZE;
		V(sem_id, 2);
		V(sem_id, 0);
		count += read_num;
		if (read_num < 1024){
			break;
		}
	}
	printf("write file over: %ld bytes\n", count);
	shmdt(p_map);
	fclose(f_out);
}
