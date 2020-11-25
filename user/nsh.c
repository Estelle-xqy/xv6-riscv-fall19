#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

void execPipe(char*argv[],int argc);

int getcmd(char *buf, int nbuf){
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}


char whitespace[] = " \t\r\n\v";
char args[10][30];
 
void setargs(char *cmd, char* argv[],int* argc)
{
    // 让argv的每一个元素都指向args的每一行
    for(int i=0;i<10;i++){
        argv[i]=&args[i][0];
    }
    int i = 0; // 表示第i个word
    // int k = 0; // 表示word中第k个char
    int j = 0;
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
        while (strchr(whitespace,cmd[j])){
            j++;
        }
        argv[i++]=cmd+j;
        // 只要不是空格，就j++,找到下一个空格
        while (strchr(whitespace,cmd[j])==0){
            j++;
        }
        cmd[j]='\0';
    }
    argv[i]=0;
    *argc=i;
}

void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            // 如果遇到 | 即pipe，至少说明后面还有一个命令要执行
            execPipe(argv,argc);
        }
         // 此时是仅处理一个命令：现在判断argv[1]开始，后面有没有> 
        // 如果遇到 > ，说明需要执行输出重定向，首先需要关闭stdout
        if(!strcmp(argv[i],">")){
            close(1);
            // 此时需要把输出重定向到后面给出的文件名对应的文件里
            open(argv[i+1],O_CREATE|O_WRONLY);
            argv[i]=0;
        }
        if(!strcmp(argv[i],"<")){
            // 如果遇到< ,需要执行输入重定向，关闭stdin
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0], argv);
}
 
void execPipe(char*argv[],int argc){
    int i=0;
    // 首先找到命令中的"|",然后把他换成'\0'
    // 从前到后，找到第一个就停止，后面都递归调用
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
    }
    // 先考虑最简单的情况：cat file | wc
    int fd[2];
    pipe(fd);
    if(fork()==0){
        // 子进程 执行左边的命令 把自己的标准输出关闭
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv,i);
    }else{
        // 父进程 执行右边的命令 把自己的标准输入关闭
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv+i+1,argc-i-1);
    }
}

int main(){
    static char buf[100];
    while(getcmd(buf, sizeof(buf)) >= 0){
    if(fork() == 0){
        char* argv[10];
        int argc=-1;
        setargs(buf, argv,&argc);
        runcmd(argv,argc);
    }
    wait(0);
  }
  exit(0);
}