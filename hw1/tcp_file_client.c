#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

struct finfo {
    char fname[256];
    u_int32_t fsize;
};

void recv_file(int sock,struct finfo info){
    char buffer[BUF_SIZE];
    int read_cnt;
    int offset = 0;

    FILE* f;
    if((f = fopen(info.fname,"wb"))==NULL){
        error_handling("file open failed");
    }

    while((read_cnt = read(sock, buffer, BUF_SIZE))!=0){
        offset+=fwrite(buffer, 1, read_cnt,f);
        if(offset>=info.fsize) break;
    }

    fclose(f);   
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

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

    struct finfo* recv_finfo;
    recv_finfo = (struct finfo*)malloc(sizeof(struct finfo) * count); 
    for(int i=0;  i<count; i++){
        int offset = 0;
        char buffer[sizeof(struct finfo)];
        while((recv_len = recv(sock,buffer, sizeof(struct finfo),0))>0){
            memcpy(&recv_finfo[i]+offset, buffer, recv_len);
            offset += recv_len;
            if(offset>=sizeof(struct finfo)) break;
        }
    }
    
    // recv_len == recv(sock,recv_finfo,sizeof(recv_finfo),0);
    // if(recv_len == -1){
    //     error_handling("recv() error 2");
    // }

    while(1){
        printf("----------list of file----------\n");
        for(int i=0; i<count; i++){
            printf("%3d %20s %d\n",i,recv_finfo[i].fname, recv_finfo[i].fsize);
        }

        int num;
        printf("\nchoose number which you want to receive a file from server\n");
        printf("if you want to quit, enter -1\n");
        
        scanf("%d", &num);
        send(sock, &num, sizeof(num),0);
        if(num == -1){
            break;
        }
        //fwrite하기
        recv_file(sock,recv_finfo[num]);
    }

    free(recv_finfo);
    close(sock);
    return 0;
}