
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
```

- **path:** The file's path relative to the root directory.
- **top_ten:** A vector containing the top 10 most used words in the file.
- **read:** Number of real-time read operations.
- **write:** Number of real-time write operations.
- **has_permission:** Specifies if the process has permission to access the file.
- **size:** The file's size in bytes.
- **no_reads and no_writes:** Conditional variables for synchronization during read and write operations.
- **next:** Pointer to the next file in the list.
- has a global **start_node** and every operation made an the list's elements has to me atomic, assured by sincronisation using the mutex `mutex_list`, so that no thread acceses an inconsistent state of the list
- the conditional variables: **no_reads** & **no_writes**; have the purpose of helping the sincronisation of read and write operations made on the a file at a centain moment; when the program makes a read operation on a file, if `write`>0 then it waits for the conditional variable no_writes to be signaled, if there are no writes, just reads, the program only increments the read value and continues with the operation, after it is finished, the `read` is decremented and the no_read is signaled if the read is 0; when the program wants to execute a write operation on a file it first verifies the `write` number to be 0, if not, is wait for the conditional variable no_writes and then increments the `write`, then, if there are more that 0 reads waits for them to end and the conditional variable no_reads to be signaled and then procedes with the operation being carefull to decrement the `write` and signal the no_writes variable after it finishes.

## Logging

Logging is implemented using the thread-safe function `write_message_log`. The log file, named "activity.log," is automatically created in the program's current directory. The file is opened with append mode and closed after each log operation to ensure file integrity.

## Inspiration Sources

- The search functionality is implemented with structures and functions from `freq_analysis.h` and `freq_analysis.c` (adapted from [GeeksforGeeks](https://www.geeksforgeeks.org/find-the-k-most-frequent-words-from-a-file/)).

- The function `create_path` is a modified version of the function found at [Stack Overflow](https://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix) for creating a file and its path simultaneously.

## Running the Server


Default configuration with a maximum of 5 connections and the root directory set to "root."
```bash
./server
```

Set the maximum connections to `nr`
```bash
./server -c <nr>
```

Set the root directory to `dir`, where dir had to be an existing directory with a valid path (eather relative or absolute)
```bash
./server -d <dir>
```
Set both the maximum connections and root directory
```bash
/server -c <nr> -d <dir>
```

## Run the client
Before running the client, make sure to change the IP_ADDRESS to match the server's current IP address.
```bash
./client
```

## Tests
After connecting to the server using the client, you can test various commands:

- Print the file list.
```
LIST       
```
- Download the file or print an error message.
```
DOWNLOAD <file_path> 
```
- Upload content to a file.
```
UPLOAD <file_path> <content>   
```
- Delete a file or print an error message.
 ```
DELETE <file_path>       
```
- Move a file or print an error message.
``` 
MOVE <file_path_source> <file_path_destination>   
```
- Update a file or print an error message.
```
UPDATE <start_poz> <content>               
```

Print files containing the specified word or nothing if none are found. Note that a file may contain a word without being in the top ten words.

**SEARCH** <word>                                       

## Makefile
To create the server and client executable files run in terminal:
```bash
make
```
To clean, tun in terminal:
```bash
make clean
```
