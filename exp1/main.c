#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

pid_t p1, p2;
int pipefd[2];	//pipefd[0]只能用于读，pipefdp[1]只能用于写

void stop(int sig_no){
	if (sig_no == SIGUSR1){
		close(pipefd[1]);
		printf("\nChild Process 1 is Killed by Parent!\n");
	}else if (sig_no == SIGUSR2){
		close(pipefd[0]);
		printf("\nChild Process 2 is Killed by Parent!\n");
	}	
	exit(0);
}

void killChild(){
	kill(p1, SIGUSR1);
	kill(p2, SIGUSR2);
}

int main(void){
	signal(SIGINT, killChild);
	if (pipe(pipefd) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);	
	}
	p1 = fork();
	if (p1 == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (p1 == 0){
		signal(SIGINT, SIG_IGN);
		signal(SIGUSR1, stop);
		close(pipefd[0]);
		int x = 1;
		char temp[30];
		memset(temp, 0, sizeof(temp));
		while(1){
			sprintf(temp, "I send you %d times.", x);
			write(pipefd[1], &temp, sizeof(temp));
			x++;
			sleep(1);
		}
	}else {	//父进程
		p2 = fork();
		if (p2 == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}	
		if (p2 == 0){	//p2进程
			signal(SIGINT, SIG_IGN);
			signal(SIGUSR2, stop);
			char temp[30];
			memset(temp, 0, sizeof(temp));
			close(pipefd[1]);
			while(1){
				while(read(pipefd[0], &temp, sizeof(temp)) > 0){
					printf("%s\n", temp);
					//setbuf(stdout, NULL);
				}
			}
			
		}else{	//父进程
			wait(NULL);
			wait(NULL);
			close(pipefd[0]);
			close(pipefd[1]);
			printf("Parent Process is Killed!\n");
		}
	}
	return 0;
}

