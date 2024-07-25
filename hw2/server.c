#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

struct frame{
	int frame_kind; //ACK:0, SEQ:1 FIN:2
	int sq_no;
	int ack;
	char data[BUF_SIZE];
	int real_size;
};

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int serv_sock;
	char message[BUF_SIZE];
	int str_len;
	socklen_t clnt_adr_sz;
	int data_total=0;
	clock_t start, finish;
	
	struct sockaddr_in serv_adr, clnt_adr;
	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (serv_sock == -1)
		error_handling("UDP socket creation error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	int frame_id = 0;

	FILE *fp;
	if((fp = fopen("img.JPG","wb"))==NULL)
		error_handling("file open failed");

	
	start=clock();
	while (1) 
	{
		clnt_adr_sz = sizeof(clnt_adr);
		struct frame frame_recv;
		struct frame frame_send;
	
		int recv_len = recvfrom(serv_sock, &frame_recv, sizeof(struct frame),0,(struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		if(recv_len > 0 && frame_recv.sq_no==frame_id && frame_recv.frame_kind==1){
			printf("[+]%dFrame Received: \n", frame_recv.sq_no); 
			int len = fwrite(frame_recv.data,1,frame_recv.real_size,fp); 
			printf("fwrite_len: %d, real_size: %d\n",len,frame_recv.real_size);
			data_total += len;

			frame_send.sq_no = 0;
			frame_send.frame_kind = 0;
			frame_send.ack = frame_recv.sq_no+1;
			sendto(serv_sock, &frame_send, sizeof(frame_send), 0, (struct sockaddr*)&clnt_adr,clnt_adr_sz);
			printf("[+]Ack Send\n");
			frame_id++;
		}
		else if(frame_recv.frame_kind==2) {
			int frame_kind = 0;
			sendto(serv_sock,&frame_send,sizeof(frame_send), 0,(struct sockaddr*)&clnt_adr,clnt_adr_sz);
			break;
		}
		else {
			printf("[+]Frame Not Received\n");
		}
	}	
	fclose(fp);
	finish = clock();



	printf("Throughout: %0.3f",(float)data_total/((double)(finish-start)/CLOCKS_PER_SEC));
	close(serv_sock);
	return 0;
}