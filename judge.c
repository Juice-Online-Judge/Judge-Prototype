#include<fcntl.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/resource.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<time.h>
#include<unistd.h>
#define MXLine 256
#define MXBuf 4096

int child_setlimits(int,int);
void strsplit(char*[],char*,char*);

int main(int argc,char *argv[]){
	//Declare Variables
	int i,ret;
	int f_time=1,f_mem=10,f_file=0;
	char cmd[MXLine],f_tmp[MXLine],cwd[MXLine];
	char *f_in = NULL,*f_out = NULL,*f_code = NULL,*f_args = NULL;
	char *args[16] = {NULL};
	
	//Parse Arguments
	for(i=1;i<argc;i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
				case 't': f_time = atoi(argv[++i]); break;
				case 'm': f_mem = atoi(argv[++i]); break;
				case 'f': f_file = atoi(argv[++i]); break;
				case 'a': f_args = argv[++i]; break;
				case 'i': f_in = argv[++i]; break;
				case 'o': f_out = argv[++i]; break;
			}
		}else f_code = argv[i];
	}
	if(f_in == NULL) f_in = "t.in";
	if(f_out == NULL) f_out = "t.out";
	if(f_code == NULL) exit(82);
	
	//Analyzing Arguments
	args[0] = f_tmp;
	if(f_args) strsplit(&args[1],f_args," ");
	
	//Compile Code
	srand(time(NULL));
	getcwd(cwd,MXLine);
	snprintf(f_tmp,MXLine,"%s/tmp_%d",cwd,rand() % 4096);
	snprintf(cmd,MXLine,"gcc %s -o %s",f_code,f_tmp);
	ret = system(cmd);
	if(ret != 0) exit(103);
	
	//Prepare For Running
	int mypipe[2];
	pid_t pid=0;
	struct timeval starttime,endtime;
	void TLE_Recieve(int signo){
		if(pid > 0) kill(pid,SIGKILL);
		unlink(f_tmp);
		exit(102);
	}
	if(signal(SIGALRM,TLE_Recieve) == SIG_ERR) exit(86);
	if(pipe(mypipe)) exit(88);
	
	//Running Code
	alarm(f_time);
	gettimeofday(&starttime,0);
	if((pid = fork()) < 0) exit(87);
	else if(pid == 0){
		if((ret = child_setlimits(f_mem,f_file)) != 0) exit(ret);
		int fd = open(f_in,O_RDONLY,0644);
		if(fd < 0) exit(89);
		if(dup2(fd,STDIN_FILENO) < 0) exit(89);
		if(dup2(mypipe[1],STDOUT_FILENO) < 0) exit(90);
		
		ret = execvp(f_tmp,args);
		exit(ret);
	}
	close(mypipe[1]);
	
	//Check For Errors
	wait(&ret);
	pid = 0;
	gettimeofday(&endtime,0);
	unlink(f_tmp);
	if(ret == 11) exit(104);
	if(ret != 0) exit(ret);
	int rtime = 1000 * (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec) / 1000;
	printf("%d ms\n",rtime);
	
	//Check For Answer
	int n1,n2,ofile = open(f_out,O_RDONLY,0644);
	char buf1[MXBuf],buf2[MXBuf];
	if(ofile < 0) exit(91);
	while(1){
		n1 = read(mypipe[0],buf1,MXBuf);
		n2 = read(ofile,buf2,MXBuf);
		if(n1 != n2) break;
		if(memcmp(buf1,buf2,n1) != 0) break;
		if(n1 != MXBuf){
			ret = 1;
			break;
		}
	}
	close(ofile);
	if(ret == 1) exit(100);
	return 101;
}

int child_setlimits(int mem,int fil){
	struct rlimit limmem = {mem<<20,mem<<20};
	struct rlimit limfile = {fil+7,fil+7};
	if(setrlimit(RLIMIT_AS,&limmem)) return 84;
	if(setrlimit(RLIMIT_NOFILE,&limfile)) return 85;
	return 0;
}

void strsplit(char *arr[],char *str,char *del){
	int i=0;
	char *s = strtok(str,del);
	while(s){
		arr[i++] = s;
		s = strtok(NULL,del);
	}
	arr[i] = NULL;
}
