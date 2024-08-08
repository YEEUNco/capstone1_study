#include "trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

struct trie* make_node(char data)
{
    struct trie *node = (struct trie*)calloc(1, sizeof(struct trie));
    for(int i=0; i<ALPHA; i++)
        node->children[i] = NULL;

    node->end_of_word = 0;
    node->data = tolower(data);
    printf("make_node: %c\n",node->data);
    node->word_list = vector_create();

    return node;
}

void free_node(struct trie *node)
{
    for(int i=0; i<ALPHA; i++)
    {
        if(node->children[i]!=NULL)
            free_node(node->children[i]);
    }
    vector_destroy(node->word_list);
    free(node);
}

struct trie* insert(struct trie *root, char *word, int freq, char *sentence)
{
    struct trie *temp = root;

    for(int i=0; i<strlen(word); i++)
    {
        int idx;
        if(word[i] != ' ')
            idx = tolower(word[i])-'a';
        else idx = ALPHA-1;

        if(temp->children[idx]==NULL)
            temp->children[idx] = make_node(word[i]);
        
        temp = temp->children[idx];
    }
    temp->end_of_word = 1;

    struct data *data = (struct data*)malloc(sizeof(struct data));
    strcpy(data->word,sentence);
    data->freq = freq;
    if(strcmp(word,sentence) == 0) // 일치할 경우
    {
        if(vector_size(temp->word_list) == 0) 
		{
			vector_push(temp->word_list, data);
			printf("check data: %s, %d\n", data->word, data->freq);
		}
    }
    else
    {
        vector_push(temp->word_list, data);
		printf("check data: %s, %d\n", data->word, data->freq);
    }
    return root;
}

//freq를 desc으로 sorting
int compare_freq(const void *a, const void *b)
{
    struct data *A = (struct data *)a;
    struct data *B = (struct data *)b;
    return B->freq - A->freq;
}

//모든 단어 collect하기
void collect(struct trie *node, vector results, int *result_count)
{
    printf("collect\n");
    if(node->end_of_word)
    {
        int size = vector_size(node->word_list);
        for(int i=0; i<size; i++)
        {
            struct data *data = (struct data *)vector_get(node->word_list, i);
            vector_push(results,data);
            (*result_count)++;
        }
    }
    for(int i=0; i<ALPHA; i++)
    {
        if(node->children[i]!=NULL)
            collect(node->children[i],results,result_count);
    }
}

struct trie * search(struct trie *root, char *prefix)
{
    struct trie *temp = root;
    for(int i=0; i<strlen(prefix); i++)
    {

        printf("search: %c\n", prefix[i]);
        int idx;
        if(prefix[i]!=' ')
        {
            idx = tolower(prefix[i])-'a';
        }
        else idx = ALPHA - 1;

        if(temp->children[idx] == NULL)
        {
            printf("search not found");
            return NULL;
        }
        temp = temp->children[idx];
    
    }
    printf("search :%c\n", temp->data);

    return temp;
    
}

void set_data(struct trie *root)
{
    FILE *fp;
    if((fp = fopen("text.txt", "r"))==NULL)
    {
        error_handling("file open failed");
    }
    char line[2048];
    while(fgets(line, sizeof(line), fp))
    {
        char sentence[SEARCH_SIZE];
		char split[SEARCH_SIZE];
        char *word;
        int freq;
        int flag = 1;
        
        sscanf(line, "%[^\t]%d", sentence, &freq);
        insert(root, sentence,freq, sentence);
        //단어로 분리
		strcpy(split, sentence);
        word = strtok(split," ");
        while(word != NULL)
        {
            printf("flag: %d\n", flag);
            if(flag)
            {
                flag = 0;
                word = strtok(NULL," ");
                continue;
            }
            printf("**%s\n", word);
            insert(root,word,freq,sentence);
            word = strtok(NULL," ");
        }
    }
    fclose(fp);
}

