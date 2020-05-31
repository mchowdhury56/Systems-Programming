/*******************************************************************************
 * Name        : chatclient.c
 * Author      : Marjan Chowdhury
 * Description : TCP/IP Chat Client Implementation
 ******************************************************************************/
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() { 
    int getStringResult = get_string(outbuf,MAX_MSG_LEN);
    if(getStringResult == TOO_LONG){
        fprintf(stderr,"Sorry, limit your message to %d characters.\n",MAX_MSG_LEN);
        fflush(stdout);
    }else if (getStringResult == OK){
        if((send(client_socket,outbuf,strlen(outbuf),0))== -1){
        fprintf(stderr, "Warning: Failed to send message to server. %s.\n",
                strerror(errno));
        }
        if(!strncmp(outbuf,"bye",3)){
            printf("Goodbye.\n");
            return -1;
        }
    }
    return EXIT_SUCCESS;
}

int handle_client_socket() {
    int bytes_recvd;
    if((bytes_recvd = recv(client_socket,inbuf,BUFLEN,0))<0){
        fprintf(stderr, "Warning: Failed to receive incoming message. %s.\n",
                strerror(errno));
    }else if(bytes_recvd == 0){
        fprintf(stderr, "\nConnection to server has been lost.\n");
        return EXIT_FAILURE; 
    }else{
        inbuf[bytes_recvd] = '\0';
        if(!strncmp(inbuf,"bye",3)){
            printf("\nServer initiated shutdown.\n");
            return -1; 
        }else{
            printf("\n%s\n",inbuf);
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(stderr,"Usage: %s <server IP> <port>\n",argv[0]);
        return EXIT_FAILURE;
    }

    int bytes_recvd, retval = EXIT_SUCCESS;
    struct sockaddr_in serv_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, addrlen);
    
    int ip_conversion = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    if (ip_conversion == 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        retval =  EXIT_FAILURE;
        goto EXIT;
    } else if (ip_conversion < 0) {
        fprintf(stderr, "Error: Failed to convert IP address. %s.\n",
                strerror(errno));
        retval =  EXIT_FAILURE;
        goto EXIT;
    }
    serv_addr.sin_family = AF_INET;

    int port;
    if (!parse_int(argv[2], &port, "port number")) {
        return EXIT_FAILURE;
    }
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Error: port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;
    }

    serv_addr.sin_port = htons(port);
    bool usernameValid = false;
    int getStringResult = NO_INPUT;
    while(!usernameValid){
        printf("Enter your username: ");
        fflush(stdout);
        getStringResult = get_string(username,MAX_NAME_LEN);
        if(getStringResult == TOO_LONG){
            fprintf(stderr, "Sorry, limit your username to %d characters.\n",MAX_NAME_LEN);
        }else if (getStringResult == OK){
            usernameValid = true;
        }
    }
    
    printf("Hello, %s. Let's try to connect to the server.\n",username);
    if ((client_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n",
                strerror(errno));
        retval =  EXIT_FAILURE;
        goto EXIT;
    }
    
    if((connect(client_socket,(struct sockaddr *)&serv_addr,addrlen)) == -1){
        fprintf(stderr, "Error: Failed to connect to server. %s.\n",
                strerror(errno));
        retval =  EXIT_FAILURE;
        goto EXIT;
    }
    
    if((bytes_recvd = recv(client_socket,inbuf,BUFLEN,0))<0){
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }else if(bytes_recvd == 0){
        fprintf(stderr, "All connections are busy. Try again later.\n");
        return EXIT_FAILURE;
    }else{
        printf("\n%s\n\n",inbuf);
    }


    if((send(client_socket,username,strlen(username),0))== -1){ 
        fprintf(stderr, "Error: Failed to send username to server. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }
    fd_set fds;
    while(true){
        FD_ZERO(&fds);
        FD_SET(client_socket,&fds);
        FD_SET(STDIN_FILENO,&fds);
        printf("[%s]: ", username);
        fflush(stdout);
        if((select(client_socket+1, &fds, NULL, NULL, NULL)) == -1){
            fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
            return EXIT_FAILURE;
        }
        
        if ((FD_ISSET(client_socket, &fds))){
            retval = handle_client_socket();
        }
        if (retval == EXIT_FAILURE || retval == -1){
            goto EXIT;
        }
        if ((FD_ISSET(STDIN_FILENO, &fds))){
            retval = handle_stdin();
        }
        if (retval == EXIT_FAILURE || retval == -1){
            goto EXIT;
        } 
    }
    
    EXIT:
        if (fcntl(client_socket, F_GETFD) >= 0) {
            close(client_socket);
        }
        if(retval == -1){
            retval = EXIT_SUCCESS;
        }
        return retval;
}
