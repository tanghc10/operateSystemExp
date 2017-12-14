#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <error.h>

#define SHMKEY 100
#define SEMKEY 6667
#define SIZE 10 //缓冲区个数

/*
	缓冲区结构
	val: 数据
	is_last: 是否是文件的结尾
*/
typedef struct Item{
	char val[1024];
	int is_last;
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

int sem_id = 0;
int shm_id = 0;
int out = 0, in = 0;

int main(int argc, char** argv){
	if (argc < 3){
		printf("请输入源目标文件， 目的目标文件，并以空格隔开\n");
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
	union semun sem_args;
	sem_args.val = SIZE;	//空缓冲区个数置为SIZE
	if (semctl(sem_id, 0, SETVAL, sem_args) == -1){
		perror("semctl error");
		return -1;
	}
	sem_args.val = 0;	//满缓冲区个数置为0
	if (semctl(sem_id, 1, SETVAL, sem_args)){
		perror("semctl error");
		return -1;
	}
	sem_args.val = 1;	//互斥信号灯置为1
	if (semctl(sem_id, 2, SETVAL, sem_args)){
		perror("semctl error");
		return -1;
	}
	/*创建两个进程*/
	pid_t p_readbuf, p_writebuf;
	if ((p_writebuf = fork()) == 0){	
		/*写进程*/
		execl("./writebuf", "writebuf", argv[1], NULL);
		return 0;
	}else{
		if ((p_readbuf = fork()) == 0){	
			/*读进程*/
			execl("./readbuf", "readbuf", argv[2], NULL);
			return 0;
		}else{
			/*等待两个进程运行结束*/
			wait(NULL);
			wait(NULL);
			/*删除信号灯*/
			if (semctl(sem_id, 0, IPC_RMID, sem_args) == -1){
				perror("semctl error");
				return -1;
			}
			/*删除共享内存组*/
			if (shmctl(shm_id, 0, IPC_RMID) == -1){
				perror("shmctl error");
				return -1;
			}
		}
	}
	return 0;
}
