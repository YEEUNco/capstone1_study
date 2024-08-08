#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "trie.h"

#define EPOLL_SIZE 50
#define SEARCH_SIZE 1024


struct client_data{
    int clnt_sock;
    struct trie *root;
    char search_word[SEARCH_SIZE];
};


void* handle_client(void *arg)
{
    struct client_data *data = (struct client_data*)arg;
    int clnt_sock = data->clnt_sock;
    struct trie *root = data->root;
    char search_word[SEARCH_SIZE] = "";
    vector results;
    int result_count = 0;
    //글자를 하나씩 받아서 처리
    while(1)
    {
        char temp;
        int read_cnt = read(clnt_sock, &temp, sizeof(char));
        printf("temp: %c\n", temp);
        if(temp == 61)
        {
            close(clnt_sock);
            free(data);
            break;
        }
        else if(temp == 92)
        {
            memset(search_word, 0, sizeof(search_word));
            continue;
        }
        else
        {
            strncat(search_word,&temp,1);
            printf("search_word: %s\n", search_word);

            results = vector_create();
            result_count = 0;
            //search함수를 통해서 search_word의 마지막 노드를 받는다
            struct trie *get = search(root,search_word);
            if(get == NULL)
            {
                printf("skip");
                write(clnt_sock, &result_count, sizeof(result_count));
                continue;
            }
            collect(get,results,&result_count);
            for(int i=0; i<vector_size(results); i++)
            {
                struct data *d = (struct data *)vector_get(results, i);
                printf("vector: %s\n", d->word);
            }

            qsort(vector_get(results,0),result_count,sizeof(struct data*), compare_freq);
            printf("result_count: %d\n", result_count);

            char final[SEARCH_SIZE]={0};
            if(result_count>10)
                result_count = 10;
            write(clnt_sock, &result_count, sizeof(result_count));
            for(int i=0; i<result_count; i++)
            {
                struct data *data = (struct data*)vector_get(results, i);
                strcpy(final,data->word);
                write(clnt_sock,final,sizeof(final));
            }
            vector_destroy(results);
        }
    }
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;

    struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;
    int opt = 1;

    char search_word[SEARCH_SIZE]="";
    struct trie *root = make_node('\0');


    serv_sock = socket(PF_INET, SOCK_STREAM, 0);  
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt,sizeof(opt));

    memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

    set_data(root);

    pthread_t t_id;
    int check=0;
    
    while(1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1) 
        {
            error_handling("accept() error");
        }

        printf("Connected client: %d\n", clnt_sock);

        struct client_data *data = (struct client_data*)malloc(sizeof(struct client_data));
        data->clnt_sock = clnt_sock;
        data->root = root;
        data->search_word[0]='\0';

        pthread_create(&t_id,NULL,handle_client,data);
        pthread_detach(t_id);

    }
    close(serv_sock);
    free_node(root);

    return 0;
}