#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>

#define KB 1024

/*用来维护输出格式*/
struct parameter{
	int nlink_length;
	int user_name_length;
	int group_name_length;
	int file_size_length;
}Limit_group;

char path[512];	

void getDirInfo(DIR *dir){
	struct dirent *entry;
	struct stat statbuf;
	unsigned long tol_size = 0;
	//blksize_t tol_size = 0;
	unsigned short max_st_nlink = 0;
	unsigned long max_st_size = 0;
	unsigned long blksize = 0;
	memset(&Limit_group, 0, sizeof(Limit_group));	//清空输出信息控制结构
	while((entry = readdir(dir)) != NULL){
		if (lstat(entry->d_name, &statbuf) != -1){
			/*跳过两个目录项"."和".."*/
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			/*通过id获取名字*/
			struct passwd *pUser = getpwuid(statbuf.st_uid);
			int user_name_length = strlen(pUser->pw_name);
			if (Limit_group.user_name_length < user_name_length) Limit_group.user_name_length = user_name_length;
			struct group *pGroup = getgrgid(statbuf.st_gid);
			int group_name_length = strlen(pGroup->gr_name);
			if (Limit_group.group_name_length < group_name_length) Limit_group.group_name_length = group_name_length;
			if (max_st_size < statbuf.st_size) max_st_size = statbuf.st_size;
			if (max_st_nlink < statbuf.st_nlink) max_st_nlink = statbuf.st_nlink;
			/*要加的是 块数*块 的大小 而不是文件大小*/
			if (blksize == 0)	blksize = statbuf.st_blksize;
			tol_size += (int)(ceil((double)statbuf.st_size / statbuf.st_blksize) * statbuf.st_blksize);
			//tol_size += statbuf.st_blocks * statbuf.st_blksize;
		}
	}
	/*通过数字得到对应的输出长度*/
	int nlink_length = (max_st_nlink == 0 ? 1 : (int)(log(max_st_nlink)/log(10) + 1));
	if (Limit_group.nlink_length < nlink_length) Limit_group.nlink_length = nlink_length;
	int file_size_length = (max_st_size == 0 ? 1 : (int)(log(max_st_size)/log(10) + 1));
	if (Limit_group.file_size_length < file_size_length) Limit_group.file_size_length = file_size_length;

	printf("total %lu\n", tol_size / KB);
	rewinddir(dir);	//设置dp目录流的读取位置回到开头处
}

/*文件的权限信息和类型信息*/
void printMode(unsigned short st_mode){
	/*与文件类型屏蔽字S_IFMT与操作*/
	switch(st_mode & S_IFDIR){
		case S_IFIFO: putchar('p'); break;
		case S_IFCHR: putchar('c'); break;
		case S_IFDIR: putchar('d'); break;
		case S_IFBLK: putchar('b'); break;
		case S_IFREG: putchar('-'); break;
		case S_IFLNK: putchar('l'); break;
		case S_IFSOCK: putchar('s'); break;
		default: putchar('-'); break;
	}
	/*与各个文件访问权限与操作得到权限*/
	if (st_mode & S_IRUSR) putchar('r'); else putchar('-');
	if (st_mode & S_IWUSR) putchar('w'); else putchar('-');
	if (st_mode & S_IXUSR) putchar('x'); else putchar('-');
	if (st_mode & S_IRGRP) putchar('r'); else putchar('-');
	if (st_mode & S_IWGRP) putchar('w'); else putchar('-');
	if (st_mode & S_IXGRP) putchar('x'); else putchar('-');
	if (st_mode & S_IROTH) putchar('r'); else putchar('-');
	if (st_mode & S_IWOTH) putchar('w'); else putchar('-');
	if (st_mode & S_IXOTH) putchar('x'); else putchar('-');
	putchar(' ');
}
 
/*文件的硬连接的数目*/
void printNlink(unsigned short st_nlink){
	printf("%*u ", Limit_group.nlink_length, st_nlink);
}

/*文件所属的用户和用户组*/
void printID(unsigned short st_uid, unsigned short st_gid){
	struct passwd *pUser = getpwuid(st_uid);
	printf("%*s ", Limit_group.user_name_length, pUser->pw_name);
	struct group *pGroup = getgrgid(st_gid);
	printf("%*s ", Limit_group.group_name_length, pGroup->gr_name);
}

/*文件大小*/
void printFileSize(unsigned long st_size){
	printf("%*lu ", Limit_group.file_size_length, st_size);
}

/*文件修改时间*/
void printTime(unsigned long mtime){
	struct tm *time = localtime(&mtime);
	printf("%2d月 %2d %2d:%2d ", time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min);
}

/*文件名称*/
void printFileName(char *filename){
	printf("%s", filename);
}

void printDir(char *dir, int depth, char ctl){
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	char *temp = NULL;
	/*调用opendir打开目录dir*/
	if ((dp = opendir(dir)) == NULL){
		printf("opendir: fail to open dir \" %s \"\n", dir);
		return;
	}
	if (chdir(dir) == -1){	//将dir设置为当前目录
		printf("chdir: fail to set \" %s \"\n", dir);
		return;
	}

	/*递归时输出目录路径*/
	if (ctl == 'R' || ctl == 'r'){
		/*通过getcwd函数获取目录路径*/
		if (getcwd(path, sizeof(path)) == NULL){
			printf("getcwd: fail\n");
			return;
		}
		printf("%s:\n", path);
	}

	/*遍历目录更新输出信息控制结构中的数据和计算文件所占空间的总和*/
	if (ctl != 'R') getDirInfo(dp);

	/*循环输出当前目录下的所有文件(包括目录)的信息*/
	while((entry = readdir(dp)) != NULL){
		if (lstat(entry->d_name, &statbuf) != -1){
			if (S_ISDIR(statbuf.st_mode)){	//判断是否是目录
				//跳过两个目录项"."和".."
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
			}
			if (ctl != 'R'){
				printMode(statbuf.st_mode);
				printNlink(statbuf.st_nlink);
				printID(statbuf.st_uid, statbuf.st_gid);
				printFileSize(statbuf.st_size);
				printTime(statbuf.st_mtime);
			}
			printFileName(entry->d_name);
			if (ctl != 'R') printf("\n");
			else printf("  ");
		}
	}
	if (ctl == 'R') printf("\n\n");
	else putchar('\n');
	rewinddir(dp);	//设置dp目录流的读取位置回到开头处

	/*递归输出当前目录下的各个子目录下的文件*/
	while ((entry = readdir(dp)) != NULL && ctl != 'l'){
		if (entry->d_type == DT_DIR){
			//跳过两个目录项"."和".."
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			printDir(entry->d_name, depth + 1, ctl);
			if (chdir("..") == -1){	//回到上层目录
				printf("chdir: fail to set \" %s \"\n", dir);
			}
		}
	}
	if (temp != NULL){
		free(temp);
	}

	/*关闭目录流*/
	if (closedir(dp) == -1){
		printf("closedir: fail to close %s\n", dir);
	}
}

int main(int argc, char *argv[]){
	char *dir = ".";
	char ctl = 'l';
	if (argc > 1){
		if (argc > 3){
			printf("too many argument!\n");
			return 0;
		}
		if (argv[1][0] == '-'){
			ctl = argv[1][1];
			if (argc == 3){
				dir = argv[2];
			}
		}else{
			dir = argv[1];
		}
	}
	printDir(dir, 1, ctl);
	return 0;
}