#include "user/user.h"
#include "../kernel/types.h"
#include "kernel/stat.h"

int main(){
    //parent_fd：写端父进程，读端子进程
    //child_fd；写端子进程，读端父进程
    int parent_fd[2],child_fd[2];
    //来回传输的数组
    char ping[] = {'p','i','n','g'};
    char pong[] = {'p','o','n','g'};
    //传输字符数组的长度
    long len_ping = sizeof(ping);
    long len_pong = sizeof(pong);
    //父进程写，子进程读的pipe
    pipe(parent_fd);
    //子进程写，父进程读的pipe
    pipe(child_fd);
    //子进程
    if(fork() == 0){
        //关掉不用的parent_fd[1]、child_fd[0]
        close(parent_fd[1]);
        close(child_fd[0]);
		//子进程从parent_fd的读端，读取字符数组
		if(read(parent_fd[0], ping, len_ping) != len_ping){
			printf("parent--->child read error!");
			exit();
		}
		//打印读取到的字符数组
		printf("%d: received ping\n", getpid());
		//子进程向child_fd的写端，写入字符数组
		if(write(child_fd[1], pong, len_pong) != len_pong){
			printf("parent<---child write error!");
			exit();
		}
        exit();
    }
    //父进程
    //关掉不用的parent_fd[0]、child_fd[1]
    close(parent_fd[0]);
    close(child_fd[1]);
	//父进程向parent_fd的写端，写入字符数组
	if(write(parent_fd[1], ping, len_ping) != len_ping){
		printf("parent--->child write error!");
		exit();
	}
	//父进程从child_fd的读端，读取字符数组
	if(read(child_fd[0], pong, len_pong) != len_pong){
		printf("parent<---child read error!");
		exit();
	}
	//打印读取的字符数组
	printf("%d: received pong\n", getpid());
    //等待进程子退出
    wait();
	exit();
}