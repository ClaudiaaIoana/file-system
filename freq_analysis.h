#ifndef FREQ_ANALYSIS_H
#define FREQ_ANALYSIS_H

#define _XOPEN_SOURCE               700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <linux/types.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>


#define MAX_CHARS 26
#define MAX_WORD_SIZE 30

//https://www.geeksforgeeks.org/find-the-k-most-frequent-words-from-a-file/
// A Trie node
struct TrieNode {
    bool isEnd; // indicates end of word
    unsigned frequency; // the number of occurrences of a word
    int indexMinHeap; // the index of the word in minHeap
    struct TrieNode* child[MAX_CHARS]; // represents 26 slots each for 'a' to 'z'.
};
typedef struct TrieNode TrieNode;

// A Min Heap node
struct MinHeapNode {
    struct TrieNode* root; // indicates the leaf node of TRIE
    unsigned frequency; // number of occurrences
    char* word; // the actual word stored
};
typedef struct MinHeapNode MinHeapNode;

// A Min Heap
struct MinHeap {
    unsigned capacity; // the total size a min heap
    int count; // indicates the number of slots filled.
    struct MinHeapNode* array; // represents the collection of minHeapNodes
};
typedef struct MinHeap MinHeap;

struct TrieNode* newTrieNode();
struct MinHeap* createMinHeap(int capacity);
void swapMinHeapNodes(MinHeapNode* a, MinHeapNode* b);
void minHeapify(MinHeap* minHeap, int idx);
void buildMinHeap(MinHeap* minHeap);
void insertInMinHeap(MinHeap* minHeap, TrieNode** root, const char* word);
void insertUtil(TrieNode** root, MinHeap* minHeap, const char* word, const char* dupWord);
void insertTrieAndHeap(const char* word, TrieNode** root, MinHeap* minHeap);
int compareMinHeapNodes(const void* a, const void* b);
void displayMinHeap(MinHeap* minHeap);
char** printKMostFreq(FILE* fp, int k, int* numWords);
void freeWords(char** words, int numWords);
void freeMinHeap(MinHeap* minHeap);

#endif // FREQ_ANALYSIS_H

//https://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix
int create_path(const char*rootDir, const char*filePath);