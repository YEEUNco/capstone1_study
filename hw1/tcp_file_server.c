#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>

struct finfo{
    char fname[256];
    u_int32_t fsize;
};

void error_handling(char *message);
int file_count();
void get_file_list(struct finfo *list);
void send_file(int clnt_sock, char *file_name);

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);   

    memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	
	bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(serv_sock, 5);
	
	clnt_addr_size=sizeof(clnt_addr);  
	clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_addr,&clnt_addr_size);
	if(clnt_sock==-1)
		error_handling("accept() error");  

    //파일 목록 읽어옴
    int count = file_count();
    struct finfo list[count];
    get_file_list(list);

    // //debug list 안에 잘 들어왔음?
    // for(int i=0; i<count;i++){
    //     printf("%3d %20s %d\n",i,list[i].fname, list[i].fsize);
    // }
    
    //send file 갯수
    if(send(clnt_sock, &count, sizeof(count),0)==-1){
        error_handling("send() error1");
    }
    
    //list struct보냄
    if(send(clnt_sock, list, sizeof(list), 0) == -1){
        error_handling("send() error2");
    }
    //read
    int num;
    if(recv(clnt_sock,&num, sizeof(num),0) == -1){
        error_handling("recv() error1");
    }
    
    //해당 파일 전송
    send_file(clnt_sock, list[num].fname);
    

    close(clnt_sock);
    close(serv_sock);
    return 0;
}

//원래는 struct finfo list[1000]; 이렇게 했는데 client에서 받을 때 문제 발생
int file_count(){
    DIR *d;
    d = opendir(".");
    struct dirent *dir;
    int count = 0;
    if(d){
        while((dir=readdir(d))!=NULL){
            if(strcmp(dir->d_name, ".")==0 || strcmp(dir->d_name, "..")==0) continue;
            count++;
        }
        closedir(d);
    }
    return count;
}

void get_file_list(struct finfo *list){
    DIR *d;
    d = opendir(".");
    struct dirent *dir;
    int count=0;
    if(d){
        while((dir = readdir(d)) != NULL){
            if(strcmp(dir->d_name, ".")==0 || strcmp(dir->d_name, "..")==0) continue;
            struct finfo info;
            strcpy(info.fname, dir->d_name);
            //debug
            // printf("%s\n", info.fname);
            //근데 방법 이거 뿐인가? 파일 다 읽어보는 방법 밖에 없나?
            FILE *f;
            f = fopen(dir->d_name, "r");
            fseek(f, 0, SEEK_END);
            info.fsize = ftell(f);
            //debug
            // printf("%d\n", info.fsize);
            list[count] = info;
            count++;
        }
        closedir(d);
    }
    return;
}

void send_file(int clnt_sock, char *file_name){
    int buffer_size=1024;
    char buff[buffer_size];
    long file_size;
    size_t result;
    FILE *f;

    if((f=fopen(file_name,"rb"))==NULL){
        error_handling("file open failed");
    }

    while (1) {
        int read_cnt = fread((void*)buff, 1, buffer_size,f);
        if(read_cnt < buffer_size){
            //debug
            // printf("%s\n", buff);
            write(clnt_sock, buff, read_cnt);
            break;
        }
        write(clnt_sock, buff, buffer_size);
    }

    fclose(f);
}

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}