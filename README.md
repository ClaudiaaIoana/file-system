
# File System Server

This project implements a multithreaded server for a file system application and includes a client for testing purposes. The server is designed to work with any client program that adheres to the specified protocol.

## Server Functionalities

The server consists of three main threads:

1. **Listening Thread:** Implemented with the `connection_thread` function, this thread listens for incoming connections on the server socket. It creates a new thread to handle each connection using the `handle_requests` function. If the maximum number of connections is not reached, a new connection is accepted. If the limit is reached, it sends a 0x8 status to the client and closes the socket.

2. **Exit Thread:** Uses epoll to wait for events signaling a clean exit, such as receiving SIGINT, SIGTERM, or writing "quit" to standard input. This thread ensures a graceful termination by closing connection threads and waiting for all connections to terminate before deallocating resources and closing the update thread.

3. **Update Thread:** Monitors the root directory, updating the file list structure (`List_f`) whenever a file system operation modifies the files. It uses a conditional variable (`updated`) and a global variable (`modif_file`) for synchronization. The thread cleans and refreshes the file list whenever it wakes up.

## Structures Used

The core structure is `List_f`:

```c
struct List {
    char *path;
    char top_ten[10][MAX_WORD_SIZE + 1];
    int read;
    int write;
    bool has_permission;
    uint32_t size;
    pthread_cond_t no_reads;
    pthread_cond_t no_writes;
    struct List *next;
};
typedef List_f;
path: The file's path relative to the root directory.
top_ten: A vector containing the top 10 most used words in the file.
read: Number of real-time read operations.
write: Number of real-time write operations.
has_permission: Specifies if the process has permission to access the file.
size: The file's size in bytes.
no_reads and no_writes: Conditional variables for synchronization during read and write operations.
next: Pointer to the next file in the list.
Logging
Logging is implemented using the thread-safe function write_message_log. The log file, named "activity.log," is automatically created in the program's current directory. The file is opened with append mode and closed after each log operation to ensure file integrity.

Inspiration Sources
The search functionality is implemented with structures and functions from freq_analysis.h and freq_analysis.c (adapted from GeeksforGeeks).

The function create_path is a modified version of the function found at Stack Overflow for creating a file and its path simultaneously.

Running the Server
bash
Copy code
# Default configuration with a maximum of 5 connections and the root directory set to "root."
./server

# Set the maximum connections to <nr>.
./server -c <nr>

# Set the root directory to <dir>.
./server -d <dir>
Running the Client
Before running the client, make sure to change the IP_ADDRESS to match the server's current IP address.

bash
Copy code
# Run the client
./client
Tests
After connecting to the server using the client, you can test various commands:

LIST: Print the file list.
DOWNLOAD <file_path>: Download the file or print an error message.
UPLOAD <file_path> <content>: Upload content to a file.
DELETE <file_path>: Delete a file or print an error message.
MOVE <file_path_source> <file_path_destination>: Move a file or print an error message.
UPDATE <start_poz> <content>: Update a file or print an error message.
SEARCH <word>: Print files containing the specified word or nothing if none are found. Note that a file may contain a word without being in the top ten words.