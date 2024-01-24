#include "freq_analysis.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
// A utility function to create a new Trie node
struct TrieNode* newTrieNode()
{
    // Allocate memory for Trie Node
    struct TrieNode* trieNode = (struct TrieNode*)malloc(sizeof(struct TrieNode));

    // Initialize values for new node
    trieNode->isEnd = false;
    trieNode->frequency = 0;
    trieNode->indexMinHeap = -1;
    for (int i = 0; i < MAX_CHARS; ++i)
        trieNode->child[i] = NULL;

    return trieNode;
}

// A utility function to create a Min Heap of given capacity
struct MinHeap* createMinHeap(int capacity)
{
    // Allocate memory for Min Heap
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));

    minHeap->capacity = capacity;
    minHeap->count = 0;

    // Allocate memory for array of min heap nodes
    minHeap->array = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode) * minHeap->capacity);

    return minHeap;
}
// A utility function to swap two min heap nodes. This
// function is needed in minHeapify
void swapMinHeapNodes(MinHeapNode* a, MinHeapNode* b)
{
    MinHeapNode temp = *a;
    *a = *b;
    *b = temp;
}
 
// This is the standard minHeapify function. It does one
// thing extra. It updates the minHapIndex in Trie when two
// nodes are swapped in in min heap
void minHeapify(MinHeap* minHeap, int idx)
{
    int left, right, smallest;
 
    left = 2 * idx + 1;
    right = 2 * idx + 2;
    smallest = idx;
    if (left < minHeap->count
        && minHeap->array[left].frequency
               < minHeap->array[smallest].frequency)
        smallest = left;
 
    if (right < minHeap->count
        && minHeap->array[right].frequency
               < minHeap->array[smallest].frequency)
        smallest = right;
 
    if (smallest != idx) {
        // Update the corresponding index in Trie node.
        minHeap->array[smallest].root->indexMinHeap = idx;
        minHeap->array[idx].root->indexMinHeap = smallest;
 
        // Swap nodes in min heap
        swapMinHeapNodes(&minHeap->array[smallest],
                         &minHeap->array[idx]);
 
        minHeapify(minHeap, smallest);
    }
}
 
// A standard function to build a heap
void buildMinHeap(MinHeap* minHeap)
{
    int n, i;
    n = minHeap->count - 1;
 
    for (i = (n - 1) / 2; i >= 0; --i)
        minHeapify(minHeap, i);
}
 
// Inserts a word to heap, the function handles the 3 cases
// explained above
void insertInMinHeap(MinHeap* minHeap, TrieNode** root,
                     const char* word)
{
    // Case 1: the word is already present in minHeap
    if ((*root)->indexMinHeap != -1) {
        ++(minHeap->array[(*root)->indexMinHeap].frequency);
 
        // percolate down
        minHeapify(minHeap, (*root)->indexMinHeap);
    }
 
    // Case 2: Word is not present and heap is not full
    else if (minHeap->count < minHeap->capacity) {
        int count = minHeap->count;
        minHeap->array[count].frequency = (*root)->frequency;
        minHeap->array[count].word  = (char*)malloc(sizeof(char)*(strlen(word)+1));
        strcpy(minHeap->array[count].word, word);
 
        minHeap->array[count].root = *root;
        (*root)->indexMinHeap = minHeap->count;
 
        ++(minHeap->count);
        buildMinHeap(minHeap);
    }
 
    // Case 3: Word is not present and heap is full. And
    // frequency of word is more than root. The root is the
    // least frequent word in heap, replace root with new
    // word
    else if ((*root)->frequency > minHeap->array[0].frequency) {
 
        minHeap->array[0].root->indexMinHeap = -1;
        minHeap->array[0].root = *root;
        minHeap->array[0].root->indexMinHeap = 0;
        minHeap->array[0].frequency = (*root)->frequency;
 
        // delete previously allocated memory and
        free(minHeap->array[0].word);
        minHeap->array[0].word = (char*)malloc(strlen(word)+1);
        strcpy(minHeap->array[0].word, word);
 
        minHeapify(minHeap, 0);
    }
}
 
// Inserts a new word to both Trie and Heap
void insertUtil(TrieNode** root, MinHeap* minHeap, const char* word, const char* dupWord)
{
    // Base Case
    if (*root == NULL)
        *root = newTrieNode();

    // There are still more characters in word
    if (*word != '\0' && (isalpha(*(word+1)) || tolower(*(word+1)) > 'a') && (*(word+1)!='.'))
    {   
		insertUtil(&((*root)->child[tolower(*word) - 97]), minHeap, word + 1, dupWord);
	}else // The complete word is processed
    {
        // word is already present, increase the frequency
        if ((*root)->isEnd)
            ++((*root)->frequency);
        else {
            (*root)->isEnd = 1;
            (*root)->frequency = 1;
        }
 
        // Insert in min heap also
        insertInMinHeap(minHeap, root, dupWord);
    }
}
 
// add a word to Trie & min heap. A wrapper over the
// insertUtil
void insertTrieAndHeap(const char* word, TrieNode** root, MinHeap* minHeap)
{
    insertUtil(root, minHeap, word, word);
}
 
// Comparison function for qsort
int compareMinHeapNodes(const void* a, const void* b)
{
    return ((struct MinHeapNode*)b)->frequency - ((struct MinHeapNode*)a)->frequency;
}

// The main function that takes a file as input, add words
// to heap and Trie, finally shows result from heap
char** printKMostFreq(FILE* fp, int k, int* numWords)
{
    // Create a Min Heap of Size k
    MinHeap* minHeap = createMinHeap(k);

    // Create an empty Trie
    TrieNode* root = NULL;
 
    // A buffer to store one word at a time
    char buffer[MAX_WORD_SIZE];
 
    // Read words one by one from file. Insert the word in
    // Trie and Min Heap
    while (fscanf(fp, "%s", buffer) != EOF)
        {
            insertTrieAndHeap(buffer, &root, minHeap);
        }

    // Sort the min heap array in descending order based on frequency
    qsort(minHeap->array, minHeap->count, sizeof(struct MinHeapNode), compareMinHeapNodes);

    // Allocate memory for the vector of words
    char** words = (char**)malloc(k * sizeof(char*));
    if (words == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Copy words from Min Heap to the vector
    *numWords = minHeap->count;
    for (int i = 0; i < minHeap->count; ++i) {
        words[i] = strdup(minHeap->array[i].word);
    }

    // Clean up and free Min Heap
    freeMinHeap(minHeap);
    return words;
}

void freeWords(char** words, int numWords)
{
    for (int i = 0; i < numWords; ++i) {
        free(words[i]);
    }
    free(words);
}

void freeMinHeap(MinHeap* minHeap)
{
    for (int i = 0; i < minHeap->count; ++i) {
        free(minHeap->array[i].word);
    }
    free(minHeap->array);
    free(minHeap);
}


int create_path(const char*rootDir, const char*filePath)
{
    // Construct the full path by combining the root directory and file path
    char fullPath[256]; 
    snprintf(fullPath, sizeof(fullPath), "%s%s", rootDir, filePath);

    // Create the directories if they don't exist
    char *dirPath = strdup(fullPath);
    assert(dirPath && *dirPath);
    
    for (char* p = strchr(dirPath + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(dirPath, 0777) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                free(dirPath);
                return -1;
            }
        }
        *p = '/';
    }
    free(dirPath);

    // Open the file for writing and return the file descriptor
    int fileDescriptor = open(fullPath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        // Handle the error as needed
    }

    return fileDescriptor;
}