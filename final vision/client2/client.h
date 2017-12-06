// this is a client

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include<unistd.h>
#include<pthread.h>

#define IP "127.0.0.1"
#define PORT 8080
#define BACKLOG 4
#define BUFFLEN 1024
#define LINEBUFF 2048
#define Name_Length 32
#define Opt_length 32
#define LENGTH 512
#define CLIENTNUM 8
#define CHATNUM 4    //the maximum chat that can exist

struct PACKAGE {
    char option[Opt_length];   //instruction
    char name[Name_Length];      //client's name
    char buff[BUFFLEN]; 
    int  clientnum;     //the number of client
};

struct THREADINFO {
    pthread_t thread_num;   //thread number
    char name[Name_Length];      //client's name
    int  clientnum;     // the number of client
    int  chatnum;       //the Number of the channel that chat in. 0 means the client is waiting, '-1'means the wait queue is full, the client can't wait.
    int  socketfd;    //describe the socket file
};

struct USER {
    int socketfd;
    char name[Name_Length];
    int clientnum;
};
