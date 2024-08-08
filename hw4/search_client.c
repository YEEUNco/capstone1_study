#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define SEARCH_WORD 1024
#define COLOR_GREEN "\033[38;2;64;165;120m"
#define COLOR_BLUE "\033[38;2;61;194;236m"
#define COLOR_RESET	"\033[0m"


void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void move_up(int lines)
{
	printf("\x1b[%dA", lines);
}

void move_down(int lines)
{
	printf("\x1b[%dB", lines);
}

void move_left_up(int lines)
{
    printf("\x1b[%dA\r", lines);
}

void move_left_down(int lines)
{
    printf("\x1b[%dB\r", lines);
}

void move_right(int count)
{
    printf("\x1b[%dC", count);
}

void move_left(int count)
{
    printf("\xB[%dD", count);
}

void clear_line()
{
	printf("\x1b[2K");
}

void to_lowercase(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        str[i] = tolower((unsigned char) str[i]);
    }
}

void colored(char *result, char *search_word)
{
	int search_len = strlen(search_word);
    char lower_result[SEARCH_WORD]="";
    char lower_search[SEARCH_WORD]="";
    int offset;
    char *pos;
    to_lowercase(strcpy(lower_result, result));
    to_lowercase(strcpy(lower_search, search_word));
	pos = strstr(lower_result,lower_search);
    offset = pos - lower_result;
    pos = result + offset;
	printf("%.*s", offset, result);
	printf("%s%.*s%s", COLOR_BLUE, search_len, pos, COLOR_RESET);
	printf("%s", pos + search_len);
}


int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    int read_cnt;

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
	

	char c;
	char search_word[SEARCH_WORD]={0};
    int result_count = 0;
    // printf("%ld\n", strlen(search_word));
	while(1) 
    {
        system("stty raw"); 
        printf("Search word: %s%s%s", COLOR_GREEN, search_word, COLOR_RESET);
        move_left_down(1);
        printf("-----------------------------------------");
        move_left_up(1);
        move_right(strlen("Search word: ")+strlen(search_word));
        c = getchar();
        if (c == 13) {
            clear_line();
			memset(search_word,0,sizeof(search_word));
            move_left_down(1);
            move_left_up(1);
            printf("Enter q to quit: ");
            c = getchar();
            if(c == 'q' || c == 'Q') {
                c = 61;
                write(sock, &c, sizeof(char));
                system("stty cooked");
                break;
            }
            move_left_down(1);
            move_left_up(1);
            c = 92;
            write(sock, &c, sizeof(char));
            clear_line();
            system("stty raw"); 
            continue;
        }

        strncat(search_word, &c, 1);
        write(sock, &c, sizeof(char));
        sleep(1);

        // printf("count: %d\n",result_count);

        move_left_down(2);
        for(int i = 0; i < result_count; i++) 
        {
            clear_line();
            move_left_down(1);
            // printf("\n");
        }
        if(result_count != 0)   
            move_left_up(result_count);

        read(sock, &result_count, sizeof(result_count));


        for(int i = 0; i < result_count; i++) 
        {
            char result[SEARCH_WORD] = {0};
            read(sock, result, sizeof(result));
            colored(result, search_word);
            // printf("%s", result);
            move_left_down(1);
            // colored(tolower(*result), tolower(*search_word));
        }

        if(result_count != 0)
            move_left_up(2+result_count);
        else
            move_left_up(2);
    }


	system("stty cooked");
    close(sock);
    return 0;
}