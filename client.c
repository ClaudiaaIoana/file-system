#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/stat.h> 
#include <signal.h>
#include <pthread.h>
#include <sys/signalfd.h>

#define PORT 8080
#define IP_ADDRESS "192.168.226.128"
#define BUFFER_SIZE 1024

void printStringWithNewline(int length, const char* str) {
    for (int i = 0; i < length; i++) {
        if (str[i] == '\0') {
            printf("\n");
        } else {
            printf("%c", str[i]);
        }
    }
}

bool check_status(uint32_t status)
{
    switch (status)
    {
        case 0x0:
            return true;
        case 0x1:
            printf("File not found\n");
            break;
        case 0x2:
            printf("Permision denied!\n");
            break;
        case 0x4:
            printf("Out of memory\n");
            break;
        case 0x8:
            printf("Server busy!\n");
            break;
        case 0x10:
            printf("Unkown operation!\n");
            break;
        case 0x20:
            printf("Bad argument!\n");
            break;
        case 0x40:
            printf("Other error!\n");
            break;
        default:
            printf("undefined behaviour!\n");
            break;
    }
    return false;
}

void list(int socket)
{
    uint32_t code_to_send = 0x0;
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    if(!check_status(status))
    {
        return;
    }

    uint32_t length;
    if (recv(socket, &length, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buffer[length];
    if (recv(socket, buffer, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    printStringWithNewline(length,buffer);
}

void download(int socket)
{
    uint32_t code_to_send=0x1;
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }
    char filename[64];
    scanf("%s",filename);
    uint32_t file_length=strlen(filename);
    filename[file_length]='\0';

    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, sizeof(filename), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    if(!check_status(status))
    {
        return;
    }

    uint32_t length;
    if (recv(socket, &length, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buffer[length];
    if (recv(socket, buffer, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    strcpy(filename,strrchr(filename,'/')+1);
    FILE* receivedFile = fopen(filename, "w");

    if (receivedFile == NULL) {
        perror("Error opening file");
    }
    fwrite(buffer, 1, length, receivedFile);

    fclose(receivedFile);
}

void upload(int socket)
{
    uint32_t code_to_send=0x2;
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    char filename[64];
    scanf("%s",filename);
    uint32_t file_length=strlen(filename);

    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, file_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    scanf("%s",buffer);
    uint32_t length=strlen(buffer);
    if (send(socket, &length, sizeof(length), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    if (send(socket, buffer, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    if(!check_status(status))
    {
        return;
    }

}

void delete(int socket)
{
    uint32_t code_to_send=0x4;
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    char filename[64];
    scanf("%s",filename);
    uint32_t file_length=strlen(filename);

    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, file_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Recv failed");
        exit(EXIT_FAILURE);
    }
    
    check_status(status);
}

void move(int socket)
{
    uint32_t code_to_send=0x8;

    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char filename[64];
    scanf("%s",filename);
    uint32_t file_length=strlen(filename);

    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, file_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    scanf("%s",filename);
    file_length=strlen(filename);
    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, file_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    check_status(status);
}

void search(int socket)
{
    uint32_t code_to_send = 0x20;
    uint32_t word_len;
    char word[64];
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    scanf("%s",word);
    word_len=strlen(word);

    if (send(socket, &word_len, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    if (send(socket, word, word_len, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    if(!check_status(status))
    {
        return;
    }

    uint32_t length;
    if (recv(socket, &length, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char buffer[length];
    if (recv(socket, buffer, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    printStringWithNewline(length,buffer);
}

void update(int socket)
{
    uint32_t code_to_send=0x10;
    if (send(socket, &code_to_send, sizeof(uint32_t), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    char filename[64];
    scanf("%s",filename);
    uint32_t file_length=strlen(filename);

    if (send(socket, &file_length, sizeof(file_length), 0) < 0) {
    perror("Send failed");
    exit(EXIT_FAILURE);
    }

    if (send(socket, &filename, file_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    uint32_t start;
    scanf("%d",&start);
    if (send(socket, &start, sizeof(start), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    char content[64];
    scanf("%s",content);
    uint32_t length=strlen(content);
    if (send(socket, &length, sizeof(length), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    if (send(socket, content, length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    uint32_t status;
    if (recv(socket, &status, sizeof(uint32_t), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    
    if(!check_status(status))
    {
        return;
    }

}

void* handle_gracious_exit(void* arg)
{
    int                     *socket = (int*)arg;
    struct epoll_event      ev;
    int                     sfd;
    struct signalfd_siginfo fdsi;
    sigset_t                mask;
 
    /* create epoll descriptor */
    int epfd = epoll_create1(0);
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);                /* CTRL-C */
    sigaddset(&mask, SIGTERM);         
 
    sfd = signalfd(-1, &mask, 0);

    ev.data.fd = sfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    struct epoll_event ret_ev;

    while (1)
    {
        epoll_wait(epfd, &ret_ev, 1, -1);

        if ((ret_ev.data.fd == sfd) && ((ret_ev.events & EPOLLIN) != 0))
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

    close(*socket);
    exit(0);
}


int main() 
{
    sigset_t mask;
 
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM); 

    sigprocmask(SIG_BLOCK, &mask, NULL);

    int client_socket;
    struct sockaddr_in server_address;
    uint32_t code_to_send = 0x0;

    // Create a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    pthread_t threadId;
    if (pthread_create(&threadId, NULL, handle_gracious_exit, &client_socket) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    pthread_detach(threadId);

    // Set up the server address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IP_ADDRESS, &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[2];
    struct epoll_event ev;
    ev.events = EPOLLIN;

    // Add client socket to epoll
    ev.data.fd = client_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
        perror("Epoll control failed");
        exit(EXIT_FAILURE);
    }

    // Add standard input to epoll
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
        perror("Epoll control failed");
        exit(EXIT_FAILURE);
    }

    printf("INTRODUCE A OPERATION:\n");

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, 2, -1);
        if (nfds == -1) {
            perror("Epoll wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == client_socket) {
                uint32_t status;
                if (recv(client_socket, &status, sizeof(uint32_t), 0) < 0) {
                    perror("Send failed");
                    exit(EXIT_FAILURE);
                }
                if(status==0x8)
                {
                    printf("The server is busy!\n");
                }
                else printf("An error occured!\n");
                break;
            } else if (events[i].data.fd == STDIN_FILENO) {
                // Standard input is ready for reading
                char oper[64];
                scanf("%s",oper);

                if(strcmp(oper,"LIST")==0)
                {
                    list(client_socket);
                }
                else if(strcmp(oper,"DOWNLOAD")==0)
                { 
                    download(client_socket);
                }
                else if(strcmp(oper,"UPLOAD")==0)
                {
                    upload(client_socket);
                }else if(strcmp(oper,"DELETE")==0)
                {
                    delete(client_socket);
                }
                else if(strcmp(oper,"MOVE")==0)
                {
                    move(client_socket);
                }
                else if(strcmp(oper,"SEARCH")==0)
                {
                    search(client_socket);
                }
                else if(strcmp(oper,"UPDATE")==0)
                {
                    update(client_socket);
                }
                else{
                    uint32_t code = 0x11;
                    if (send(client_socket, &code, sizeof(uint32_t), 0) < 0) {
                        perror("Send failed");
                        exit(EXIT_FAILURE);
                    }

                    uint32_t status;
                    if (recv(client_socket, &status, sizeof(uint32_t), 0) < 0) {
                        perror("Send failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        break;
    }

    // Close the socket
    close(client_socket);

    return 0;
}
