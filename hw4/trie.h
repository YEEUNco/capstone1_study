#include "vector.h"
#include <pthread.h>

#define ALPHA 27
#define SEARCH_SIZE 1024

struct data
{
    char word[SEARCH_SIZE];
    int freq;
};

struct trie
{
    struct trie *children[ALPHA];
    int end_of_word;
    char data;
    vector word_list;
};

void error_handling(char *message);
struct trie* make_node(char data);
void free_node(struct trie *node);
struct trie* insert(struct trie *root, char *word, int freq, char *sentence);
int compare_freq(const void *a, const void *b);
void collect(struct trie *node, vector results, int *result_count);
struct trie * search(struct trie *root, char *prefix);
void set_data(struct trie *root);
