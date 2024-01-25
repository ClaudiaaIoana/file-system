
#include"freq_analysis.h"

#define EPOLL_INIT_BACKSTORE        2

#define PORT                        8080
#define MAX_CONNECTIONS             2
#define DEFAULT_DIR                 "root"

int     max_num_connections;
int     current_connections=0;
char    root_dir[32];
bool    modif_file=false;
pthread_cond_t  updated;

pthread_mutex_t mutex_con;
pthread_mutex_t mutex_list;
pthread_mutex_t mutex_update;
pthread_mutex_t mutex_log;

struct List{
    char        *path;
    char        top_ten[10][MAX_WORD_SIZE+1];
    int         read;
    int         write;
    bool        has_permision;
    uint32_t    size;
    pthread_cond_t  no_reads;
    pthread_cond_t  no_writes;
    struct List *next;
}typedef List_f;

List_f *start_node=NULL;
int     num_files=0;

void add_file(const char* filename, uint32_t size);

void remove_file(const char*filename);

List_f *get_file(const char* filename);

void clean_list();

void traverse_directory(const char* dirname);

void* handle_list(void* arg);

void* connection_thread(void* arg);

void* handle_requests(void* arg);

void* handle_gracious_exit(void* arg);

void wait_end_connections();

char* list_files(uint32_t *length);

char* list_search_files(const char *word, uint32_t *length);

void send_list(int socket, int flags);

void exec_download(int socket, int flags);

void exec_upload(int socket);

void exec_delete(int socket);

void exec_move(int socket);

void exec_search(int socket);

void exec_update(int socket);

void modified();

void write_message_log(char* message);

int main(int argc, char* argv[]) 
{

    max_num_connections=MAX_CONNECTIONS;
    strcpy(root_dir,DEFAULT_DIR);
    int opt;
    while ((opt = getopt(argc, argv, "c:d:")) != -1) {
        switch (opt) {
            case 'c':
                max_num_connections=atoi(optarg);
                break;
            case 'd':
                strcpy(root_dir,optarg);
                break;
            default:
                break;
        }
    }

    pthread_mutex_init(&mutex_con, NULL);
    pthread_mutex_init(&mutex_list, NULL);
    pthread_cond_init(&updated, NULL);
    pthread_mutex_init(&mutex_update, NULL);
    pthread_mutex_init(&mutex_log, NULL);

    sigset_t mask;
 
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);         

    sigprocmask(SIG_BLOCK, &mask, NULL);

    pthread_t thread_id[3];

    if (pthread_create(&thread_id[0], NULL, handle_list, (void*)root_dir) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    } 

    if (pthread_create(&thread_id[1], NULL, connection_thread, (void*)NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&thread_id[2], NULL, handle_gracious_exit, thread_id) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(thread_id[0], NULL);
    pthread_join(thread_id[1], NULL);
    pthread_join(thread_id[2], NULL);

    printf("Closing\n");

    return 0;
}

void* connection_thread(void* arg)
{
    int                 server_fd, new_socket;
    socklen_t           addrlen;
    struct sockaddr_in  address;
    pthread_t           thread_id;

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to a specific address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while(1)
    {
        // Accept incoming connections
        addrlen = sizeof(address);
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&mutex_con);
        if(current_connections>=max_num_connections)
        {
            pthread_mutex_unlock(&mutex_con);
            uint32_t code=0x8;
            uint32_t bytesWrite = send(new_socket, &code, sizeof(uint32_t),0);
            if (bytesWrite == -1) {
                perror("write");
            }
            close(new_socket);
        }
        else{

        pthread_mutex_unlock(&mutex_con);

        // Create a thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_requests, (void *)&new_socket) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to allow it to clean up automatically
        pthread_detach(thread_id);

        printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        }
    
    }
    // Close the socket
    close(server_fd);
}

void add_file(const char*filename, uint32_t size)
{
    char **words;
    int num_words;
    FILE*f;
    char full_path[512];
    strcpy(full_path,root_dir);
    strcat(full_path,"/");

    if(!start_node)
    {
        start_node=(List_f*)malloc(sizeof(List_f));
        start_node->path=strdup(filename);
        start_node->read=0;
        start_node->write=0;
        start_node->size=size;
        start_node->next=NULL;
        strcat(full_path,filename);
        f=fopen(full_path,"r");
        if(f==NULL)
        {
            start_node->has_permision=false;
        }
        else{
            start_node->has_permision=true;
            words=printKMostFreq(f,10,&num_words);
            for(int i=0;i<num_words;i++)
            {
                strcpy(start_node->top_ten[i],words[i]);
            }
            fclose(f);
            freeWords(words,num_words);
        }
        pthread_cond_init(&start_node->no_reads, NULL);
        pthread_cond_init(&start_node->no_writes, NULL);
        num_files++;
    }
    else
    {
        List_f *elem=(List_f*)malloc(sizeof(List_f));
        elem->path=strdup(filename);
        elem->read=0;
        elem->write=0;
        elem->next=start_node;
        elem->size=size;
        strcat(full_path,filename);
        f=fopen(full_path,"r");
        if(f==NULL)
        {
            elem->has_permision=false;
        }
        else{
            elem->has_permision=true;
            words=printKMostFreq(f,10,&num_words);
            for(int i=0;i<num_words;i++)
            {

                strcpy(elem->top_ten[i],words[i]);
            }
            fclose(f);
            freeWords(words,num_words);
        }
        pthread_cond_init(&elem->no_reads, NULL);
        pthread_cond_init(&elem->no_writes, NULL);
        start_node=elem;
        num_files++;
    }
}

void remove_file(const char*filename)
{
    if(!start_node)
    {
        return;
    }
    List_f *prev_node=NULL;
    List_f *node=start_node;
    while(node)
    {
        if(strcmp(node->path,filename)==0)
        {
            if(!prev_node)
                prev_node->next=node->next;
            else
                start_node=node->next;
            free(node->path);
            free(node);
            num_files--;
            break;
        }
        node=node->next;
    }
}

List_f *get_file(const char* filename)
{
    if(!start_node)
    {
        return NULL;
    }
    List_f *node=start_node;
    while(node)
    {
        if(strcmp(node->path,filename)==0)
        {
            return node;
        }
        node=node->next;
    }

    return NULL;
}

void clean_list()
{
    if(!start_node)
    {
        return;
    }
    List_f *next_node=NULL;
    List_f *node=start_node;
    while(node)
    {
        next_node=node->next;
        free(node->path);
        free(node);
        num_files--;
        node=next_node;
    }
    start_node=NULL;
}

void traverse_directory(const char* dirname) 
{
    DIR*            dir;
    struct dirent*  entry;
    struct stat     path_stat;
    char            path[1024];
    uint32_t        size;

    // Open the directory
    dir = opendir(dirname);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory: %s\n", dirname);
        return;
    }

    // Traverse each entry in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Ignore current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path of the entry
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        // Get information about the entry
        if (stat(path, &path_stat) != 0) {
            fprintf(stderr, "Failed to get file information: %s\n", path);
            continue;
        }

        // If the entry is a directory, recursively traverse it
        if (S_ISDIR(path_stat.st_mode)) {
            traverse_directory(path);
        }
        // If the entry is a regular file, add it to the list
        else if (S_ISREG(path_stat.st_mode)) {
            strcpy(path,strchr(path,'/'));
            size=path_stat.st_size;
            add_file(path,size);
        }
    }

    // Close the directory
    closedir(dir);
}

void printStringWithNewline(int length, const char* str) {
    for (int i = 0; i < length; i++) {
        if (str[i] == '\0') {
            printf("\n");
        } else {
            printf("%c", str[i]);
        }
    }
}

void* handle_list(void* arg)
{
    pthread_cleanup_push(clean_list, (void*)NULL);
    const char*   init_dir=(const char*)arg;

    pthread_mutex_lock(&mutex_list);
    traverse_directory(init_dir);
    pthread_mutex_unlock(&mutex_list);

    while(1)
    {
        pthread_mutex_lock(&mutex_update);
            while(!modif_file)
                pthread_cond_wait(&updated,&mutex_update);

            pthread_mutex_lock(&mutex_list);
                clean_list();
                traverse_directory(init_dir);
            pthread_mutex_unlock(&mutex_list);
            modif_file=false;
        pthread_mutex_unlock(&mutex_update);
    }
    pthread_cleanup_pop(1);
}

void* handle_requests(void* arg)
{
    pthread_mutex_lock(&mutex_con);
        current_connections++;
    pthread_mutex_unlock(&mutex_con);

    int     client_socket = *((int *)arg);
    uint32_t code;

    ssize_t bytesRead = recv(client_socket, &code, sizeof(code),0);
    if (bytesRead <= 0) {
	    perror("Client closed the connection! ");
        pthread_mutex_lock(&mutex_con);
        current_connections--;
        pthread_mutex_unlock(&mutex_con);

    close(client_socket);
        pthread_exit(0);
    }

    uint32_t status=0;
    uint32_t bytesWrite;
    switch(code)
    {
        case 0x0:
            printf("LIST\n");
            send_list(client_socket,0);
            break;
        case 0x1:
            printf("DOWNLOAD\n");
            exec_download(client_socket,0);
            break;
        case 0x2:
            printf("UPLOAD\n");
            exec_upload(client_socket);
            break;
        case 0x4:
            printf("DELETE\n");
            exec_delete(client_socket);
            break;
        case 0x8:
            printf("MOVE\n");
            exec_move(client_socket);
            break;
        case 0x10:
            printf("UPDATE\n");
            exec_update(client_socket);
            break;
        case 0x20:
            printf("SEARCH\n");
            exec_search(client_socket);
            break;
        default:
            printf("INVALID OPERATION!\n");
            status=0x10;
            bytesWrite = send(client_socket, &status, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            break;
    }

    pthread_mutex_lock(&mutex_con);
        current_connections--;
    pthread_mutex_unlock(&mutex_con);

    close(client_socket);
}

void* handle_gracious_exit(void* arg)
{
    struct epoll_event      ev;
    int                     sfd;
    struct signalfd_siginfo fdsi;
    sigset_t                mask;
    pthread_t               *threads_id=(pthread_t*)arg;
 
    /* create epoll descriptor */
    int epfd = epoll_create(EPOLL_INIT_BACKSTORE);
    
    /* read user data from standard input */
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

 
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);                /* CTRL-C */
    sigaddset(&mask, SIGTERM);         
 
    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        printf("Someting went wrong with the signal fd\n");
    }

    ev.data.fd = sfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    struct  epoll_event ret_ev;
    char    input[64];
 
    while(1)
    {
        epoll_wait(epfd, &ret_ev, 1, -1);
    
        if ((ret_ev.data.fd == STDIN_FILENO) && ((ret_ev.events & EPOLLIN) != 0))
        {
            int bytes_read=read(ret_ev.data.fd,input,sizeof(input)-1);
            input[strlen(input)-1]='\0';
            if(strcmp(input,"quit")==0)
                break;
        }
        else if ((ret_ev.data.fd == sfd) && ((ret_ev.events & EPOLLIN) != 0))
        {
            struct signalfd_siginfo fdsi;
            ssize_t s = read(ret_ev.data.fd, &fdsi, sizeof(struct signalfd_siginfo));
            if (s != sizeof(struct signalfd_siginfo))
            {
                perror("read");
                exit(EXIT_FAILURE);
            }

            if (fdsi.ssi_signo == SIGINT)
            {
                printf("Received SIGINT\n");
                break;
            }
            else if (fdsi.ssi_signo == SIGTERM)
            {
                printf("Received SIGTERM\n");
                break;
            }
        }
    }

    pthread_cancel(threads_id[1]);
    wait_end_connections();
    pthread_cancel(threads_id[0]);
    
    pthread_mutex_destroy(&mutex_con);
    pthread_mutex_destroy(&mutex_list);
    pthread_mutex_destroy(&mutex_update);
    pthread_mutex_destroy(&mutex_log);
    pthread_cond_destroy(&updated);
}

void wait_end_connections()
{
    while(1)
    {
        pthread_mutex_lock(&mutex_con);
        if(current_connections==0)
            break;
        pthread_mutex_unlock(&mutex_con);
    }
}

char* list_files(uint32_t* length) {
    char* list = NULL;
    char vector_files[num_files][256];
    uint32_t total_length = 0;
    int i = 0;

    pthread_mutex_lock(&mutex_list);

    if (!start_node) {
        *length = 0;
        pthread_mutex_unlock(&mutex_list);
        return strdup("");
    }

    List_f* node = start_node;
    while (node) {
        strcpy(vector_files[i], node->path);
        total_length += strlen(node->path) + 1;
        node = node->next;
        i++;
    }

    list = (char*)malloc(total_length * sizeof(char)); 
    char* aux = list;
    for (int j = 0; j < i; j++) {
        strcpy(aux, vector_files[j]);
        aux[strlen(vector_files[j])]='\0';
        aux += strlen(vector_files[j]) + 1;
    }

    *length = total_length; // Update the length

    pthread_mutex_unlock(&mutex_list);
    return list;
}

bool has_word(List_f* file,const char*word)
{
    for(int i=0;i<10 ;i++)
    {
        if(strcmp(file->top_ten[i],word)==0)
        {
            return true;
        }
    }
    return false;
}

char* list_search_files(const char *word, uint32_t *length)
{
    char* list = NULL;
    char vector_files[num_files][256];
    uint32_t total_length = 0;
    int i = 0;

    pthread_mutex_lock(&mutex_list);

    if (!start_node) {
        *length = 0;
        pthread_mutex_unlock(&mutex_list);
        return strdup("");
    }

    List_f* node = start_node;
    while (node) {
        if(has_word(node,word))
        {
            strcpy(vector_files[i], node->path);
            total_length += strlen(node->path) + 1; // Update total_length
            i++; 
        }
        node = node->next;
    }

    list = (char*)malloc(total_length * sizeof(char)); // Allocate memory for the concatenated string
    char* aux = list;
    for (int j = 0; j < i; j++) {
        strcpy(aux, vector_files[j]);
        aux[strlen(vector_files[j])]='\0';
        aux += strlen(vector_files[j]) + 1;
    }

    *length = total_length; // Update the length

    pthread_mutex_unlock(&mutex_list);
    return list;
}

void send_list(int socket, int flags)
{
    uint32_t len;
    char* to_send=list_files(&len);
    uint32_t code=0x0;
    ssize_t bytesWrite = send(socket, &code, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    bytesWrite = send(socket,&len,sizeof(len),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    bytesWrite = send(socket,to_send,len,0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    write_message_log("list\n");
}

void exec_download(int socket, int flags)
{
    char            file_path[256];
    uint32_t        path_len;
    uint32_t        code=0x0;
    ssize_t         bytesWrite;
    uint32_t        size;

    ssize_t bytesRead = recv(socket, &path_len, sizeof(path_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_path, path_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    file_path[path_len]='\0';

    pthread_mutex_lock(&mutex_list);
        List_f *file=get_file(file_path);
        if(!file)
        {
            code=0x1;
            bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }

            pthread_mutex_unlock(&mutex_list);
            return;
        }
        if(!file->has_permision)
        {
            code=0x2;
            bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }

            pthread_mutex_unlock(&mutex_list);
            return;
        }
        size=file->size;
        
        while (file->write>0)
        { 
            pthread_cond_wait(&file->no_writes, &mutex_list);
        }
        file->read++;
    pthread_mutex_unlock(&mutex_list);

    code=0x0;
    bytesWrite = send(socket, &code, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
    }

    bytesWrite = send(socket, &size, sizeof(size),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    char full_path[256];
    strcpy(full_path,root_dir);
    strcat(full_path,file_path);

    int fd = open(full_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
    }

    sendfile(socket, fd, NULL, size);

    pthread_mutex_lock(&mutex_list);
        file->read--;
        if(file->read==0)
            pthread_cond_signal(&file->no_reads);
    pthread_mutex_unlock(&mutex_list);
    char message[512];
    sprintf(message," download, %s\n",file_path);
    write_message_log(message);
}

void exec_upload(int socket)
{
    char            file_path[256];
    uint32_t        path_len;
    uint32_t        code=0x0;
    ssize_t         bytesWrite;
    uint32_t        size;

    ssize_t bytesRead = recv(socket, &path_len, sizeof(path_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_path, path_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    file_path[path_len]='\0';

    uint32_t length;
    if (recv(socket, &length, sizeof(length), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buffer[length];
    if (recv(socket, buffer, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    buffer[length]='\0';

    int fd=create_path(root_dir,file_path);
    bytesWrite = write(fd,buffer,length);
    if(bytesWrite<length)
    {
        //out of memory
        code=0x4;
    }
    bytesWrite = send(socket, &code, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    char message[512];
    sprintf(message," upload, %s\n",file_path);
    write_message_log(message);
    
    close(fd);
    modified();
}

void exec_delete(int socket)
{
    char            file_path[256];
    uint32_t        path_len;
    uint32_t        code=0x0;
    uint32_t        status;

    ssize_t bytesRead = recv(socket, &path_len, sizeof(path_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_path, path_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }
    file_path[path_len]='\0';

    pthread_mutex_lock(&mutex_list);
        List_f *file=get_file(file_path);
        if(!file)
        {
            status=0x1;
            ssize_t bytesWrite = send(socket, &status, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&mutex_list);
            return;
        }
        if(!file->has_permision)
        {
            code=0x2;
            uint32_t bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }
            pthread_mutex_unlock(&mutex_list);
            return;
        }
    pthread_mutex_unlock(&mutex_list);

    char    complete_path[256];
    strcpy(complete_path,root_dir);
    strcat(complete_path,file_path);
    if (rmdir(complete_path) == 0) {
        status = 0x0;
    } else if (remove(complete_path) == 0) {
        status = 0x0;
    } else {
        status = 0x40;
    }

    ssize_t bytesWrite = send(socket, &status, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    char message[512];
    sprintf(message," delete, %s\n",file_path);
    write_message_log(message);

    modified();
}

void exec_move(int socket)
{
    char            file_source[256];
    uint32_t        source_len;
    uint32_t        size;
    char            file_destination[256];
    uint32_t        destination_len;
    uint32_t        code=0x0;
    uint32_t        status=0x0;

    ssize_t bytesRead = recv(socket, &source_len, sizeof(source_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_source, source_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }
    file_source[source_len]='\0';

    bytesRead = recv(socket, &destination_len, sizeof(destination_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_destination, destination_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }
    file_destination[destination_len]='\0';

    pthread_mutex_lock(&mutex_list);
        List_f *file=get_file(file_source);
        if(!file)
        {
            status=0x1;
            uint32_t bytesWrite = send(socket, &status, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&mutex_list);
            return;
            //TOFO send status func
        }
        if(!file->has_permision)
        {
            code=0x2;
            size_t bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }

            pthread_mutex_unlock(&mutex_list);
            return;
        }
        size=file->size;
    pthread_mutex_unlock(&mutex_list);

    char    complete_source[256];
    strcpy(complete_source,root_dir);
    strcat(complete_source,file_source);


    int fd_source=open(complete_source,O_RDONLY);
    int fd_destination=create_path(root_dir,file_destination);

    char content[size];
    bytesRead=read(fd_source,content,size);
    ssize_t bytesWrite= write(fd_destination,content,size);
    if(bytesWrite<size)
    {
        status=0x4;
    }
    close(fd_destination);
    close(fd_source);

    if(remove(complete_source))
        status=0x40;

    bytesWrite = send(socket, &status, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    char message[530];
    sprintf(message," move, %s, %s\n",file_source,file_destination);
    write_message_log(message);

    modified();
}

void exec_search(int socket)
{
    uint32_t word_len;
    char word[MAX_WORD_SIZE];
    ssize_t bytesRead = recv(socket, &word_len, sizeof(uint32_t),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, word, word_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    word[word_len]='\0';

    uint32_t len;
    char* to_send=list_search_files(word,&len);
    uint32_t code=0x0;
    ssize_t bytesWrite = send(socket, &code, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    bytesWrite = send(socket,&len,sizeof(len),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    bytesWrite = send(socket,to_send,len,0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    } 

    char message[512];
    sprintf(message," search, %s\n",word);
    write_message_log(message);
}

void exec_update(int socket)
{
    char            file_path[256];
    char            full_path[256];
    uint32_t        path_len;
    uint32_t        start;
    uint32_t        size;
    char            *string;
    uint32_t        code=0x0;
    ssize_t         bytesWrite;

    strcpy(full_path,root_dir);
    strcat(full_path,"/");

    ssize_t bytesRead = recv(socket, &path_len, sizeof(path_len),0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    bytesRead = recv(socket, file_path, path_len,0);
    if (bytesRead == -1) {
	    perror("read");
        exit(EXIT_FAILURE);
    }

    file_path[path_len]='\0';
    strcat(full_path,file_path);

    if (recv(socket, &start, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    if (recv(socket, &size, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    string=(char*)malloc(size*sizeof(char));
    if (recv(socket, string, size, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    string[size]='\0';

    pthread_mutex_lock(&mutex_list);
        List_f *file=get_file(file_path);
        if(!file)
        {
            code=0x1;
            bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }

            pthread_mutex_unlock(&mutex_list);
            return;
        }
        if(!file->has_permision)
        {
            code=0x2;
            bytesWrite = send(socket, &code, sizeof(code),0);
            if (bytesWrite == -1) {
                perror("write");
            }

            pthread_mutex_unlock(&mutex_list);
            return;
        }
        while (file->write>0)
        { 
            pthread_cond_wait(&file->no_writes, &mutex_list);
        }
        file->write++;
        while(file->read>0)
        {
            pthread_cond_wait(&file->no_reads, &mutex_list);
        }

    int fd=open(full_path, O_WRONLY);
    lseek(fd,start,SEEK_SET);
    bytesWrite = write(fd,string,size);
    if(bytesWrite<size)
    {
        //out of memory
        code=0x4;
    }
    bytesWrite = send(socket, &code, sizeof(code),0);
    if (bytesWrite == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    close(fd);
    pthread_mutex_unlock(&mutex_list);

    modified();

    char message[512];
    sprintf(message," update, %s\n",file_path);
    write_message_log(message);
}

void modified()
{
    pthread_mutex_lock(&mutex_update);
        modif_file=true;
        pthread_cond_signal(&updated);
    pthread_mutex_unlock(&mutex_update);
}

void write_message_log(char* message)
{
    time_t rawtime;
    struct tm *info;
    char timestamp[20];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d, %H:%M, ", info);

    pthread_mutex_lock(&mutex_log);
    // Write the formatted date and message to the log
    FILE* log=fopen("activity.log","a");
    fprintf(log, "%s %s", timestamp, message);
    pthread_mutex_unlock(&mutex_log);
}

