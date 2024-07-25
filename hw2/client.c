#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>


#define BUF_SIZE 1024

struct frame{
	int frame_kind; //ACK:0, SEQ:1 FIN:2
	int sq_no;
	int ack;
	char data[BUF_SIZE];
	int real_size;
};

int timeout(int sock, long sec, long usec){
	struct timeval timeout;
 	timeout.tv_sec = sec;
 	timeout.tv_usec = usec;

	fd_set fds; // 나는 한번만 검사하면 되니깐 복사 안해도 되는 듯
	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	return select(sock+1, &fds, 0, 0, &timeout);
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;
	socklen_t adr_sz;
	
	struct sockaddr_in serv_adr, from_adr;
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock = socket(PF_INET, SOCK_DGRAM, 0);   
	if (sock == -1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	int frame_id = 0;
	int ack_recv = 1;

	char buff[BUF_SIZE];
	int read_cnt;
	
	FILE *fp;
	if((fp = fopen("30mb.jpg","rb"))==NULL){
		error_handling("file open failed");
	}
	
	int for_timeout=5000;
	int total =0;

	while ((read_cnt=fread((void*)buff, 1, BUF_SIZE, fp))>0)
	{
		int timeout_check;
		struct frame frame_send;
		struct frame frame_recv;
		clock_t start, finish;
		
		while(1){
			if(ack_recv==1){ // 깔끔하게 바꾸려면 이 부분을 밖으로 빼고 ack_recv=0으로 들어오게 바꾸면 됨
				frame_send.sq_no = frame_id;
				frame_send.frame_kind = 1;
				frame_send.ack = 0;
				memcpy(frame_send.data,buff,read_cnt);
				frame_send.real_size = read_cnt;
				
				sendto(sock, &frame_send, sizeof(frame_send), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
				printf("[+]Frame Send: %d\n", frame_send.sq_no);
				start = clock();
			}
			else { // 다시 보내줘야 함
				sendto(sock, &frame_send, sizeof(frame_send), 0, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
				printf("[+]Frame Send again\n");
			}

			timeout_check = timeout(sock, 0,for_timeout);

			if(timeout_check==-1)
				error_handling("timeout error");

			if(timeout_check>0){ //
				int addr_size = sizeof(serv_adr);
				int recv_len = recvfrom(sock,&frame_recv,sizeof(frame_recv), 0,(struct sockaddr*)&serv_adr, &addr_size);
				if( recv_len > 0 && frame_recv.sq_no == 0 && frame_recv.ack == frame_id+1){ //여기 sq_no고쳐야할거 같은데?
					finish = clock();
					printf("[+]Ack Received\n");
					ack_recv = 1;
					frame_id++;
					total += (finish-start);
					for_timeout = (int)(((double)total / frame_id) * 1000);// 평균값?
					printf("timeout : %d\n", for_timeout);
					if(for_timeout<13000)
						for_timeout=13000; 
					break;
				}else{
					printf("[-]Ack Not Received\n");
					ack_recv = 0;
				}	
			}
			else{ //time out
				printf("[-]Ack Not Received\n");
				ack_recv = 0;
			}
		}
	}	
	fclose(fp);
	
	struct frame frame_send;

	frame_send.frame_kind = 2;
	frame_send.sq_no = frame_id;
	frame_send.ack = 0;
	
	sendto(sock, &frame_send, sizeof(frame_send), 0,(struct sockaddr*)&serv_adr, sizeof(serv_adr));
	sendto(sock, &frame_send, sizeof(frame_send), 0,(struct sockaddr*)&serv_adr, sizeof(serv_adr));

	close(sock);
	return 0;
}

