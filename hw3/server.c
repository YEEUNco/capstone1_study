#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define PATH_SIZE 256
#define NAME 64
#define EPOLL_SIZE 50

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_count_cond = PTHREAD_COND_INITIALIZER;

struct list_info{
    int type; // file: 0 | dir: 1
    char f_name[NAME];
    int f_size;
    char d_name[NAME];
};

int thread_count = 0;

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int count_dir(char path[]){
    DIR* dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        error_handling("open dir failed at count_dir");
    }

    while ((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0) continue;
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR ) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

void ls(void *arg)
{
    int clnt_sock = *((int*) arg);
    int read_cnt = 0;
    char buf[BUF_SIZE];
    char path[PATH_SIZE];
    char origin_path[PATH_SIZE];
    int offset=0;

    struct dirent *dentry;
    struct stat statbuf;
    
    //path받기
    while((read_cnt = read(clnt_sock, buf, BUF_SIZE))>0)
    {
        memcpy(path + offset, buf, read_cnt);
        offset+=read_cnt;
        if(offset>=PATH_SIZE) break;
    }

    int count = count_dir(path);
    write(clnt_sock, &count, sizeof(count));

    if (getcwd(origin_path, sizeof(origin_path)) == NULL)
    {
        error_handling("getcwd() error");
    }

    DIR *dp;

    if((dp=opendir(path)) == NULL)
        error_handling("opening dir failed");

    if(chdir(path) == -1 )
        error_handling("changing dir failed");

    struct list_info file;

    while((dentry = readdir(dp))!=NULL)
    {
        memset(&file, 0, sizeof(struct list_info));
        if(dentry->d_ino == 0)
            continue;
        
        if(stat(dentry->d_name, &statbuf)==-1)
            error_handling("stat failed");

        if(strcmp(dentry->d_name, ".")==0 || strcmp(dentry->d_name, "..")==0) continue;

        if(S_ISREG(statbuf.st_mode)) 
        {
            file.type = 0;
            strcpy(file.f_name, dentry->d_name);
            file.f_size = statbuf.st_size;
            write(clnt_sock, &file.type,sizeof(file.type));
            write(clnt_sock, file.f_name, sizeof(file.f_name));
            write(clnt_sock, &file.f_size, sizeof(file.f_size));
            
        }
        else if(S_ISDIR(statbuf.st_mode))
        {
            file.type = 1;
            strcpy(file.d_name, dentry->d_name);
            write(clnt_sock, &file.type, sizeof(file.type));
            write(clnt_sock, file.d_name, sizeof(file.d_name));
            
        } 
    }

    closedir(dp);
    chdir(origin_path);
}

void f_download(void *arg)
{
    int clnt_sock = *((int*) arg);
    int read_cnt = 0;
    int offset = 0;
    char buf[BUF_SIZE];
    char path[PATH_SIZE];
    char file_name[NAME];

    //path받기
    while((read_cnt = read(clnt_sock, buf, BUF_SIZE))>0)
    {
        memcpy(path + offset, buf, read_cnt);
        offset+=read_cnt;
        if(offset>=PATH_SIZE) break;
    }
    
    //file name받기
    offset = 0;
    while((read_cnt = read(clnt_sock, buf, BUF_SIZE))>0)
    {
        memcpy(file_name + offset, buf, read_cnt);
        offset+=read_cnt;
        if(offset>=NAME) break;
    }

    strcat(path,"/");
    strcat(path,file_name);

    pthread_mutex_lock(&mutex);
    //file 보내주기
    FILE *fp;
    if((fp = fopen(path,"rb"))==NULL)
        error_handling("FIle open failed at f_download");

    while ((read_cnt = fread((void*)buf, 1, BUF_SIZE,fp)) > 0) {
            int written = write(clnt_sock, buf, read_cnt );
    }
    fclose(fp);
    

    pthread_mutex_unlock(&mutex);
}

void* f_upload(void *arg)
{
    int upload_sock = *((int*)arg);
    free(arg);
    int str_len;
    char path_info[PATH_SIZE];
    char buf[BUF_SIZE];
    int read_cnt;
    int offset = 0;
    int file_size;

    //path받기
    while((read_cnt = read(upload_sock, buf, BUF_SIZE))>0)
    {
        memcpy(path_info + offset, buf, read_cnt);
        offset+=read_cnt;
        if(offset>=PATH_SIZE) break;
    }
    offset = 0;
    read_cnt = read(upload_sock, &file_size, sizeof(file_size));
    
    FILE *fp;
    fp = fopen(path_info, "wb");
    if (fp == NULL) {
        perror("fopen failed");
        error_handling("file open failed at f_upload\n");
    }

    while((read_cnt = read(upload_sock, buf, BUF_SIZE))>0){
        offset+=fwrite(buf, 1, read_cnt,fp);
        if(offset>=file_size) break;
    }

    fclose(fp);
    close(upload_sock);
}

void f_upload_handler(void *arg) // 파일 받아야함
{
    int clnt_sock = *((int*) arg);
    int serv_sock;
    struct sockaddr_in serv_adr;
    int port;

    pthread_t t_id;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0); 
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);

    while(1)
    {
        port = random()%40000+20000;
        serv_adr.sin_port = htons(port);

        if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) != -1)
            break;
    }
    if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

    write(clnt_sock, &port, sizeof(port));

    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz = sizeof(clnt_adr);
    int upload_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
    if (upload_sock == -1) {
        // perror("accept() error");
        close(serv_sock);
        return;
    }

    int *upload_sock_ptr = malloc(sizeof(int));
    *upload_sock_ptr = upload_sock;
    pthread_create(&t_id, NULL, f_upload, upload_sock_ptr);
    pthread_join(t_id,NULL);
    // f_upload(upload_sock);

    close(serv_sock);
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int*) arg);
    free(arg);

    int type = 0; // dir변경 0, ls 1,download 2, upload 3
    int read_cnt = 0;

    ls(&clnt_sock);

    while(type != 4)
    {
        read_cnt = read(clnt_sock, &type, sizeof(type));
        if(read_cnt == -1)
            error_handling("read failed at handle_clnt");

        if(type == 0) // cd
        {

        }
        else if(type == 1) // ls
        {
            ls(&clnt_sock);
        }
        else if(type == 2) // download
        {
            f_download(&clnt_sock);
        }
        else if(type == 3) // upload
        {
            f_upload_handler(&clnt_sock);
        }
        else if(type == 4) {
            break;
        }
    }
    close(clnt_sock);
    pthread_mutex_lock(&thread_count_mutex);
    thread_count--;
    pthread_cond_signal(&thread_count_cond);
    pthread_mutex_unlock(&thread_count_mutex);
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd,event_cnt;

    pthread_t t_id;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);  

    memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

    epfd=epoll_create(EPOLL_SIZE);
    ep_events=malloc(sizeof(struct epoll_event)*EPOLL_SIZE);

    event.events = EPOLLIN;
    event.data.fd=serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock,&event);

    while((event_cnt=epoll_wait(epfd, ep_events, EPOLL_SIZE,-1))!=-1)
    {
        for(int i=0; i<event_cnt; i++)
        {
            if(ep_events[i].data.fd==serv_sock)
            {
                clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                
                event.events = EPOLLIN ;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);

                printf("connected client: %d \n", clnt_sock);

                int *csp = malloc(sizeof(int));
                *csp = clnt_sock;

                pthread_mutex_lock(&thread_count_mutex);
                thread_count++;
                pthread_mutex_unlock(&thread_count_mutex);

                pthread_create(&t_id, NULL, handle_clnt, csp);
                pthread_detach(t_id);
            }
        }

        pthread_mutex_lock(&thread_count_mutex);
        while (thread_count > 0) {
            pthread_cond_wait(&thread_count_cond, &thread_count_mutex);
        }
        pthread_mutex_unlock(&thread_count_mutex);

        if (thread_count == 0) {
            break;
        }
    }
    close(serv_sock);
    close(epfd);
    free(ep_events);
    return 0;
}