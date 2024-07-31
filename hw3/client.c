#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define PATH_SIZE 256
#define NAME 64

struct list_info{
    int type; // file: 0 | dir: 1
    char f_name[NAME];
    int f_size;
    char d_name[NAME];
};

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void print_file_list(int sock,int count, struct list_info *info)
{
    int type;
    printf("--------- file list ----------\n");
    for(int i=0; i<count; i++)
    {
        read(sock,&type,sizeof(type));
        info[i].type = type;
        if(info[i].type == 0) 
        {   
            read(sock, info[i].f_name, sizeof(info[i].f_name));
            read(sock, &info[i].f_size, sizeof(info[i].f_size));
            printf("%3d [file] %10s %5d\n", i+1,info[i].f_name, info[i].f_size);
        }
        else if(info[i].type == 1)
        {
            read(sock, info[i].d_name, sizeof(info[i].d_name));
            printf("%3d [dir]  %10s\n", i+1,info[i].d_name);
        }
    }
    printf("\n");
}

int print_menu()
{
    int no;

    printf("1. cd\n");
    printf("2. ls\n");
    printf("3. dowonload file\n");
    printf("4. upload file\n");
    printf("5. quit\n");
    printf("Enter number you want to do? ");
    scanf("%d", &no);
    getchar();

    return no - 1;
}

void file_download(int sock,struct list_info info)
{
    char buffer[BUF_SIZE];
    int read_cnt;
    int offset = 0;

    FILE* fp;
    if((fp = fopen(info.f_name,"wb"))==NULL){
        error_handling("file open failed");
    }

    while((read_cnt = read(sock, buffer, BUF_SIZE))>0){
        offset+=fwrite(buffer, 1, read_cnt,fp);
        if(offset>=info.f_size) break;
    }

    fclose(fp);   
}

int get_file_size(char *file){
    FILE *fp;
    if((fp = fopen(file,"rb"))==NULL)
        error_handling("file open failed at get file size");
    fseek(fp, 0, SEEK_END); 

    int size = ftell(fp); 
    
    fclose(fp);
    return size;
}

void file_upload(int sock, char *file_name){
    char buf[BUF_SIZE];
    int read_cnt;

    FILE *fp;
    if((fp = fopen(file_name,"rb"))==NULL){
        perror("fopen failed");
        error_handling("file open failed at file_download");
    }
        
    
    while ((read_cnt = fread((void*)buf, 1, BUF_SIZE,fp)) > 0) {
            int written = write(sock, buf, read_cnt);
    }
    fclose(fp);

}

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    int read_cnt;
    char buf[BUF_SIZE]={0};
    char path[PATH_SIZE] = {0};
    int type=-1;
    int offset=0;

    if(argc!=3)
    {
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

    strcpy(path,".");
    write(sock,path,sizeof(path));
    int count;
    read_cnt = read(sock,&count,sizeof(count));

    struct list_info *info;
    info = (struct list_info*)malloc(sizeof(struct list_info) * count);
    print_file_list(sock, count,info);

    while(type != 4)
    {
        while(type < 0 || type > 4)
            type = print_menu();
        write(sock, &type, sizeof(type));

        if(type == 0)
        {
            int no;
            printf("Enter a directory number you want to go\n");
            scanf("%d", &no);
            getchar();
            strcat(path,"/");
            strcat(path,info[no-1].d_name);
            //디렉토리 파일 아닐 때 error처리
            
        }
        else if(type == 1)
        {
            write(sock,path,sizeof(path));
            free(info);

            int count=0;
            read_cnt = read(sock,&count,sizeof(count));

            info = (struct list_info*)malloc(sizeof(struct list_info) * count);

            print_file_list(sock, count,info);
        }
        else if(type == 2) // download
        {
            //ls한번 실행해줘야함
            
            write(sock, path, sizeof(path));
            int no;
            printf("Enter a file number you want to download\n");
            scanf("%d", &no);
            getchar();
            //여기도 download 에러처리 필요함

            write(sock, info[no-1].f_name,sizeof(info[no-1].f_name));

            file_download(sock,info[no-1]);
        }
        else if(type == 3)
        {

            int new_port;
            read(sock, &new_port, sizeof(new_port));

            int new_sock;
            struct sockaddr_in new_serv_addr;

            new_sock=socket(PF_INET, SOCK_STREAM, 0);
            if(new_sock == -1)
                error_handling("socket() error");

            memset(&new_serv_addr, 0, sizeof(new_serv_addr));
            new_serv_addr.sin_family=AF_INET;
            new_serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
            new_serv_addr.sin_port=htons(new_port);

            if(connect(new_sock, (struct sockaddr*)&new_serv_addr, sizeof(new_serv_addr))==-1) 
                error_handling("connect() error!");
            
            char path_info[PATH_SIZE]="";
            char file_name[NAME]="hello.txt";
            strcat(path_info,path);
            strcat(path_info,"/");
            strcat(path_info,file_name);

            
            write(new_sock, path_info, sizeof(path_info));
            int size = get_file_size(file_name);
            
            write(new_sock,&size, sizeof(size));
    
            file_upload(new_sock,file_name);
        }
        else if(type == 4) break;
        type = -1;
        printf("\n");
    }


    free(info);
    close(sock);
    return 0;
}