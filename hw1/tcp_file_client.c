#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>


struct finfo {
    char fname[256];
    u_int32_t fsize;
};

void error_handling(char *message);
void recv_file(int sock,FILE *f);

int main(int argc, char* argv[]){
    int sock;
    struct sockaddr_in serv_addr;

    if(argc!=3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

    sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) 
		error_handling("connect() error!");
     


    int count;
    int recv_len = recv(sock, &count, sizeof(count),0);
    if(recv_len == -1)
        error_handling("recv() error 1");
    struct finfo recv_finfo[count];
    //여기 while문 써야함 왜 이렇게는 안되는거지?
    // while((recv_len = recv(sock,recv_finfo,sizeof(recv_finfo),0))>0){
    //     if(recv_len == -1){
    //         error_handling("recv() error 2");
    //     }
    // }
    recv_len == recv(sock,recv_finfo,sizeof(recv_finfo),0);
    if(recv_len == -1){
        error_handling("recv() error 2");
    }

    printf("----------list of file----------\n");
    for(int i=0; i<count; i++){
        printf("%3d %20s %d\n",i,recv_finfo[i].fname, recv_finfo[i].fsize);
    }
    int num;
    printf("\nchoose number which you want to receive a file from server\n");
    scanf("%d", &num);
    send(sock, &num, sizeof(num),0);

    //fwrite하기
    FILE* f;
    if((f = fopen(recv_finfo[num].fname,"wb"))==NULL){
        error_handling("file open failed");
    }
    recv_file(sock,f);


    close(sock);
    return 0;
}

void recv_file(int sock,FILE *f){
    int buff_size = 1024;
    char buffer[buff_size];
    int read_cnt;

    while((read_cnt = read(sock, buffer, buff_size))!=0){
        fwrite(buffer, 1, read_cnt,f);
    }

    fclose(f);
    
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}