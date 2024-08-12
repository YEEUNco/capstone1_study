#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "vector.h"

#define BUF_SIZE 1024

pthread_mutex_t mutex;

struct timeval  tv;
double begin, end;

struct file_info{
    int n_th;
    // int from_sender;
    char *contents;
    long file_size;
};

struct recv_info{
    int port;
    struct in_addr ip;
};

struct for_s_f_thread{ //for sending file thread
    int sock;
    struct file_info *fi;
};

struct for_I_r_thread{ // I receive file to recv 
    int my_sock;
    struct file_info **fi;
};

struct for_s_I_thread{ //sender send me file
    int send_sock;
    int will_get_f;
    int total_recv;
    struct receiver_info *ri;
    struct file_info **fi;
};

struct for_g_f_r_s_thread{
    int my_no;
    int will_get_f;
    int total_recv;
    int total_file;
    int send_sock;
    struct file_info **fi;
    struct recv_info *ri;
};

struct for_sftr_thread{
    int sock;
    struct file_info fi;
};

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int split_file(vector file_data, char* file_name, int seg_size)
{   
    FILE *fp;
    int read_cnt;
    int count = 0;
    char *buf;
    struct file_info *fi;

    seg_size *= 1024;

    if((fp=fopen(file_name, "rb")) == NULL)
    {
        error_handling("file open failed");
    }

    while(1)
    {
        buf = (char*)malloc(sizeof(char) * seg_size);
        if((read_cnt = fread(buf,1,seg_size,fp))<=0)
        {
            free(buf);
            break;
        }

        fi = (struct file_info*)malloc(sizeof(struct file_info));
        fi->n_th = count;
        count++;
        // fi->from_sender = 1;
        fi->file_size = read_cnt;
        fi->contents = buf;

        vector_push(file_data,fi);
    }
    return count;
}

void *send_file_from_sender(void *arg)
{
    struct for_s_f_thread sft = *((struct for_s_f_thread*)arg);
    struct file_info fi = *(sft.fi);
    double time_sec, Mbps;

    pthread_mutex_lock(&mutex);
    gettimeofday(&tv, NULL);
    begin = (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0;

    write(sft.sock, &(fi.n_th), sizeof(fi.n_th));
    write(sft.sock, &(fi.file_size), sizeof(fi.file_size));
    write(sft.sock, fi.contents, fi.file_size);

    gettimeofday(&tv, NULL);
	end = (tv.tv_sec) * 1000.0 + (tv.tv_usec) / 1000.0;
    time_sec = (end - begin) / 1000.0; 
    Mbps = (fi.file_size * 8.0) / (time_sec * 1000000.0);
    printf("To receving peer #%d: %fMbps (%ld Bytes Sent / %fs)\n", fi.n_th, Mbps,fi.file_size,time_sec);
    pthread_mutex_unlock(&mutex);
    
    free(sft.fi->contents);
    free(sft.fi);
    free(arg);
    return NULL;
}

void sender(int max_peer, char *file_name, int seg_size, int listen_port)
{
    vector file_data = vector_create();
    int recv_count = 0, file_count;
    pthread_t *t_id;

    int send_sock;
    int *recv_sock;
    struct sockaddr_in send_addr,recv_addr;
    socklen_t recv_addr_size;
    int option = 1;

    struct recv_info *ri;

    int rb = 0;
    struct file_info *fi;

    //파일 분리
    file_count = split_file(file_data,file_name, seg_size);

    send_sock = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(send_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	send_addr.sin_port = htons(listen_port);

    if (bind(send_sock, (struct sockaddr*) &send_addr, sizeof(send_addr)) == -1)
		error_handling("bind() error");

	if (listen(send_sock, 5) == -1)
		error_handling("listen() error");

    //receiver 접속 + receiver 정보 담기
    ri = (struct recv_info*)malloc(sizeof(struct recv_info) * max_peer);
    recv_sock = (int*)malloc(sizeof(int) * max_peer);
    recv_addr_size = sizeof(recv_addr);
    while(recv_count < max_peer)
    {
        recv_sock[recv_count] = accept(send_sock, (struct sockaddr*)&recv_addr, &recv_addr_size);
        write(recv_sock[recv_count], &recv_count, sizeof(recv_count));
        read(recv_sock[recv_count], &ri[recv_count].port,sizeof(ri[recv_count].port));
        ri[recv_count].ip = recv_addr.sin_addr;
        recv_count++;
    }

    //사전 정보 보내기(recv_count, file_count, struct recv_info)
    for(int i = 0; i < recv_count; i++)
    {
        write(recv_sock[i], &recv_count, sizeof(recv_count));
        write(recv_sock[i], &file_count, sizeof(file_count));
        for(int k = 0; k < recv_count; k++)
        {
            write(recv_sock[i], &ri[k], sizeof(ri[k]));
        }
        
    }

    pthread_mutex_init(&mutex, NULL);

    //파일 보내기
    t_id = (pthread_t *)malloc(sizeof(pthread_t)*file_count);
    for(int i = 0; i < file_count; i++)
    {
        
        if(rb == recv_count)
            rb = 0;

        struct for_s_f_thread *sft = (struct for_s_f_thread *)malloc(sizeof(struct for_s_f_thread));
        sft->sock = recv_sock[rb];
        fi = ((struct file_info*)vector_get(file_data, i));
        sft->fi = fi;

        pthread_create(&t_id[i], NULL, send_file_from_sender, sft);

        rb++;
    }

    for(int i=0; i<file_count; i++)
    {
        pthread_join(t_id[i], NULL);
    }   
    
    close(send_sock);
    vector_destroy(file_data);
    free(ri);
    free(recv_sock);
    free(t_id);
}

//recv peer로 부터 오는 파일 받는 함수
void *recv_file_from_recv(void *arg)
{
    struct for_I_r_thread Irt = *((struct for_I_r_thread*)arg);
    int my_sock = Irt.my_sock;
    struct file_info **fi_arr = Irt.fi;

    int recv_sock;

    struct sockaddr_in recv_addr;
    socklen_t recv_addr_size;

    int count;

    struct file_info *fi;
    char *buf;
    int n_th;
    long file_size;
    char temp_buf[BUF_SIZE];
    int read_cnt;

    recv_addr_size = sizeof(recv_addr);
    recv_sock = accept(my_sock, (struct sockaddr*)&recv_addr, &recv_addr_size);
    if(recv_sock==-1)
		error_handling("accept() error");  
    else 
        printf("conntected");
    
    //total 몇개의 파일을 받야하는지
    read(recv_sock, &count, sizeof(count));

    //file받음
    for(int i = 0; i < count; i++)
    {
        struct timeval  local_tv;
        double local_begin, local_end;
        double local_time_sec, local_Mbps;
        
        fi = (struct file_info *)malloc(sizeof(struct file_info));
        //file segment받기
        read(recv_sock, &n_th, sizeof(n_th));
        fi->n_th = n_th;
        //file size받기
        read(recv_sock, &file_size, sizeof(file_size));
        fi->file_size = file_size;
      
        //thoughtput 측정 시작
        gettimeofday(&local_tv, NULL);
        local_begin = (local_tv.tv_sec) * 1000.0 + (local_tv.tv_usec) / 1000.0;

        //파일 내용 받기
        buf = (char*)malloc(sizeof(char)*fi->file_size);
        for(int i=0; i<fi->file_size; )
        {
            if(fi->file_size - i >= BUF_SIZE)
                read_cnt = read(recv_sock, temp_buf, BUF_SIZE);
            else
                read_cnt = read(recv_sock,temp_buf,fi->file_size - i);
            
            memcpy(buf+i,temp_buf,read_cnt);
            
            i+=read_cnt;

        }
        fi->contents = buf;

        //그리고 저장
        fi_arr[fi->n_th] = fi;

        //throughput 측정 종료
        gettimeofday(&local_tv, NULL);
        local_end = (local_tv.tv_sec) * 1000.0 + (local_tv.tv_usec) / 1000.0;
        local_time_sec = (local_end - local_begin) / 1000.0; 
        local_Mbps = (fi->file_size * 8.0) / (local_time_sec * 1000000.0);
        printf("From Receiving Peer #%d: %fMbps (%ld Bytes Sent / %fs)\n", fi->n_th,local_Mbps,fi->file_size,local_time_sec);
    }

    return NULL;

}

//recv peer에게 파일 보내는 함수
void *send_file_to_recv(void *arg)
{
    struct for_sftr_thread sftr = *((struct for_sftr_thread*)arg);
    int sock = sftr.sock;
    struct file_info fi = sftr.fi;

    write(sock, &(fi.n_th), sizeof(fi.n_th));
    write(sock, &(fi.file_size), sizeof(fi.file_size));
    write(sock, fi.contents, fi.file_size);
    
    return NULL;
}

//sender에게서 파일 받는 함수
void *get_file_from_sender(void *arg)
{
    struct for_g_f_r_s_thread fgrs = *((struct for_g_f_r_s_thread*)arg);
    int will_get_f = fgrs.will_get_f;
    int send_sock = fgrs.send_sock;
    int recv_count = fgrs.total_recv;
    int file_count = fgrs.total_file;
    int my_no = fgrs.my_no;
    struct file_info **fi_arr = fgrs.fi;
    struct recv_info *ri_arr = fgrs.ri;
    struct file_info *fi;
    char *buf;
    char temp_buf[BUF_SIZE];
    struct for_sftr_thread *sftrth;

    int *sock_arr;
    struct sockaddr_in *addr_arr ;
    int option = 1;
    int count = 0;
    int n_th;
    long file_size;
    int read_cnt;

    pthread_t *t_id = (pthread_t*)malloc(sizeof(pthread_t) * (will_get_f * (recv_count-1) ));
    sftrth = (struct for_sftr_thread*)malloc(sizeof(struct for_sftr_thread) * (will_get_f * (recv_count-1)));
    
    //recv 연결을 다해준다
    sock_arr = (int *)malloc(sizeof(int) * (recv_count));
    addr_arr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * (recv_count));

    for(int i=0; i<recv_count; i++)
    {
        if(i == my_no)
            continue;
        sock_arr[i] = socket(PF_INET, SOCK_STREAM, 0);
        setsockopt(sock_arr[i], SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
        if(sock_arr[i] == -1)
		    error_handling("socket() error");
        
        memset(&addr_arr[i], 0, sizeof(addr_arr[i]));
        addr_arr[i].sin_family=AF_INET;
        addr_arr[i].sin_addr=ri_arr[i].ip;
        printf("%d IP: %s\n", i, inet_ntoa(ri_arr[i].ip));
        addr_arr[i].sin_port=htons(ri_arr[i].port);
        printf("%d PORT: %d\n", i, ri_arr[i].port);

        while(connect(sock_arr[i], (struct sockaddr*)&addr_arr[i], sizeof(addr_arr[i]))==-1);
        //몇개의 파일을 받아야하는지 알려줌
        write(sock_arr[i], &will_get_f, sizeof(will_get_f));
    }

    //sender로부터 file 받음
    for(int i =  0; i < will_get_f; i++)
    {
        struct timeval  local_tv;
        double local_begin, local_end;
        double local_time_sec, local_Mbps;
        
        fi = (struct file_info*)malloc(sizeof(struct file_info));
        
        read(send_sock, &n_th, sizeof(n_th));
        fi->n_th = n_th;
        read(send_sock, &file_size, sizeof(file_size));
        fi->file_size = file_size;

        gettimeofday(&local_tv, NULL);
        local_begin = (local_tv.tv_sec) * 1000.0 + (local_tv.tv_usec) / 1000.0;
        buf = (char*)malloc(sizeof(char)*fi->file_size);
        for(int i=0; i<fi->file_size; )
        {
            if(fi->file_size - i >= BUF_SIZE)
                read_cnt = read(send_sock, temp_buf, BUF_SIZE);
            else
                read_cnt = read(send_sock,temp_buf,fi->file_size - i);
            
            memcpy(buf+i,temp_buf,read_cnt);
            
            i+=read_cnt;

        }
        fi->contents = buf;

        fi_arr[fi->n_th] = fi;
        gettimeofday(&local_tv, NULL);
        local_end = (local_tv.tv_sec) * 1000.0 + (local_tv.tv_usec) / 1000.0;
        local_time_sec = (local_end - local_begin) / 1000.0; 
        local_Mbps = (fi->file_size * 8.0) / (local_time_sec * 1000000.0);
        printf("From Sending Peer #%d: %fMbps (%ld Bytes Sent / %fs)\n", fi->n_th,local_Mbps,fi->file_size,local_time_sec);

        
        //recv peer에게 파일 보내줌
        for(int k = 0; k < recv_count; k++)
        {
            if(k == my_no)
                continue;

            sftrth[count].sock = sock_arr[k];
            sftrth[count].fi = *fi;
            pthread_create(&t_id[count], NULL, send_file_to_recv, &sftrth[count]);
            count++;
        }
    }

    for(int i = 0; i < will_get_f * (recv_count-1); i++)
        pthread_join(t_id[i], NULL);
    
    free(t_id);
    free(arg);

    return NULL;
}

//받은 파일 조각들을 저장하는 함수
void save_file(struct file_info **fi, int file_count)
{
    FILE *fp;
    int write_cnt = 0;
    if((fp = fopen("save.jpg", "wb")) == NULL)
        error_handling("opening file failed");

    for(int i = 0; i < file_count; i++)
    {
        write_cnt = 0;
        while(write_cnt < fi[i]->file_size)
        {
            write_cnt += fwrite(fi[i]->contents+write_cnt, 1, fi[i]->file_size - write_cnt,fp);
        }
    }

    fclose(fp);
    for(int i = 0; i < file_count; i++)
    {
        free(fi[i]->contents);
        free(fi[i]);
    }
    free(fi);
}


void receiver(char *ip, int port, int listen_port)
{
    int send_sock;
    struct sockaddr_in send_addr;

    int my_no;
    int recv_count, file_count;
    struct recv_info *recv_info;

    int will_get_f;

    int my_sock;
    struct sockaddr_in my_addr;
    int option = 1;

    struct file_info **fi;
    struct for_I_r_thread Irt;
    pthread_t *Irthread;

    pthread_t rfsth;
    struct for_g_f_r_s_thread *gfrst;

    send_sock=socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(send_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	if(send_sock == -1)
		error_handling("socket() error");

    memset(&send_addr, 0, sizeof(send_addr));
	send_addr.sin_family=AF_INET;
	send_addr.sin_addr.s_addr=inet_addr(ip);
	send_addr.sin_port=htons(port);

    if(connect(send_sock, (struct sockaddr*)&send_addr, sizeof(send_addr))==-1) 
		error_handling("connect() error!");
    else printf("connected!!");


    
    //my_no받기, listen port 보내기
    read(send_sock, &my_no, sizeof(my_no));
    write(send_sock, &listen_port, sizeof(listen_port));

    //사전 정보 받기(recv_count, file_count, struct recv_info)
    read(send_sock, &recv_count, sizeof(recv_count));
    read(send_sock, &file_count, sizeof(file_count));
    recv_info = (struct recv_info*)malloc(sizeof(struct recv_info) * recv_count);
    for(int i = 0; i < recv_count; i++)
    {
        read(send_sock, &recv_info[i], sizeof(struct recv_info));
    }

    will_get_f = file_count/recv_count;
    if((my_no+1) <= (file_count % recv_count)) 
        will_get_f++;
  
    //다른 recver가 파일 보낼 수 있도록 thread 생성
    my_sock = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(my_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(listen_port);

    if (bind(my_sock, (struct sockaddr*) &my_addr, sizeof(my_addr)) == -1)
		error_handling("bind() error");

	if (listen(my_sock, 5) == -1)
		error_handling("listen() error");

    //recv로부터 파일 받기
    fi = (struct file_info**)malloc(sizeof(struct file_info*) * file_count);
    Irt.fi = fi;
    Irt.my_sock = my_sock;
    Irthread = (pthread_t *)malloc(sizeof(pthread_t) * (recv_count-1));
    for(int i = 0; i < recv_count-1; i++)
    {
        pthread_create(&Irthread[i], NULL, recv_file_from_recv ,&Irt);
    }

    //sender로부터 파일을 받고, 바로 다른 recv에게 보내줌
    gfrst = (struct for_g_f_r_s_thread*)malloc(sizeof(struct for_g_f_r_s_thread));
    gfrst->my_no = my_no;
    gfrst->will_get_f = will_get_f;
    gfrst->total_recv = recv_count;
    gfrst->total_file = file_count;
    gfrst->send_sock = send_sock;
    gfrst->fi = fi;
    gfrst->ri = recv_info;
    pthread_create(&rfsth, NULL, get_file_from_sender, gfrst);

    for(int i = 0; i<(recv_count-1) ; i++)
    {
        pthread_join(Irthread[i], NULL);
    }

    pthread_join(rfsth, NULL);

    //파일 save
    save_file(fi, file_count);
    
}


int main(int argc, char *argv[])
{
    int opt;
    int flag_s = 0, flag_n = 0, flag_f = 0, flag_g = 0; //sender
    int flag_r = 0, flag_a = 0; //receiver
    int flag_p = 0; //common
    int max_peer = 0,segment_size = 0;
    char file_name[256] = {0};
    char sender_ip[31] = {0};
    int sender_port = 0, listen_port;

    while((opt = getopt(argc, argv, "sn:f:g:ra:p:"))!=-1)
    {
        switch (opt)
        {
            case 's' : 
                flag_s = 1; 
                break;
            case 'n' : 
                flag_n = 1;
                max_peer = atoi(optarg); 
                break;
            case 'f' : 
                flag_f = 1; 
                strcpy(file_name,optarg);
                break;
            case 'g' : 
                flag_g = 1; 
                segment_size = atoi(optarg);
                break;
            case 'r' : 
                flag_r = 1; 
                break;
            case 'a' : 
                flag_a = 1; 
                sscanf(optarg, "%31[^:]:%d", sender_ip, &sender_port);
                break;
            case 'p' : 
                flag_p = 1;
                listen_port = atoi(optarg);
                break;
             default:
                printf("Usage: %s [-s] [-n max_peer] [-f file_name] [-g segment_size] [-r] [-a sender_ip:sender_port] [-p listen_port]\n", argv[0]);
                exit(0);
        }
    }

    if(flag_s){
        if(!flag_n || !flag_f || !flag_g || !flag_p)
        {
            printf("Sending peer requires -n, -f, -g, and -p options\n");
            exit(0);
        }
        sender(max_peer,file_name, segment_size,listen_port);
    }
    else if(flag_r)
    {
        if(!flag_a || !flag_p)
        {
            printf("Receiving peer requires -a and -p options\n");
            exit(0);
        }
        receiver(sender_ip, sender_port, listen_port);
    }
}