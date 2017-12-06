// this is a client

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<netdb.h>
#include"client.h"

int isconnected;
int socketfd;
char option[LINEBUFF];
struct USER user;


int connectserver();
void setname(struct USER *user);
void login(struct USER *user);
void *receiver(void *param);
void *fileReceiver(void *param);
void broadcast(struct USER *user, char *msg);
void whisp(struct USER *user, char *msg);
void transfer(struct USER *user, char *msg);


int main(int argc, char**argv) {
    int namelength;
    int sent;
    memset(&user, 0, sizeof(struct USER));
    while (gets(option)) {
        if (!strncmp(option,"connect", 7)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            memset(user.name, 0, sizeof(char) * Name_Length);
            if (ptr !=NULL) {
                namelength = strlen(ptr);
                if (namelength >Name_Length) 
                    ptr[Name_Length] = 0;
                strcpy (user.name, ptr);
            }
            else {
                strcpy(user.name, "Anonymous");
            }
            if (isconnected) {
                fprintf(stderr, "You are already connected to server...\n");
                return;
            }
            socketfd= connectserver();
            if (socketfd >= 0) {
                isconnected = 1;
                user.socketfd=socketfd;
                if (strcmp(user.name, "Anonymous"))
                    setname(&user);
                printf("Logged in as %s\n", user.name);
                printf("Receiver started [%d]...\n", socketfd);
                struct THREADINFO threadinfo;
                pthread_create(&threadinfo.thread_num, NULL, receiver, (void *)&threadinfo);
                pthread_create(&threadinfo.thread_num, NULL, fileReceiver, (void *)&threadinfo);
            }
            else {
                fprintf(stderr, "Connection rejected...\n");
            }
        }
        else if(!strncmp(option, "name", 4)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            memset(user.name, 0, sizeof(char) * Name_Length);
            if (ptr !=NULL) {
                namelength = strlen(ptr);
                if (namelength>Name_Length) {
                    ptr[Name_Length] = 0;
                }
                strcpy(user.name, ptr);
                setname(&user);
            }
        }
        else if(!strncmp(option, "help", 4)) {
            FILE *fin = fopen("help.txt", "r");
            if (fin !=NULL) {
                while(fgets(option, LINEBUFF-1, fin)) {
                    puts(option);
                }
                fclose(fin);
            }
            else {
                fprintf(stderr, "Help file not found... \n");
            }
        }
        else if(!strncmp(option, "exit", 4)) {
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected... \n");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "exit");
            strcpy(package.name, user.name);
            send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
            isconnected = 0;
            break;
        }
        else if(!strncmp(option, "chat", 4)) {
            printf("Help you find a chatter... \n");
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected... \n");
                return;
            }
                memset(&package, 0, sizeof(struct PACKAGE));
                strcpy(package.option, "chat");
                strcpy(package.name, user.name);
                strcpy(package.buff, "msg");
                package.clientnum = user.clientnum;
                send (socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
        }
        else if (!strncmp(option, "whisp", 5)) {
            whisp(&user, &option[6]);
        }
        else if (!strncmp(option, "transfer", 8)) {
           /* struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "transfer");
            strcpy(package.name, user.name);
            strcpy(package.buff, "msg");
            package.clientnum = user.clientnum;
            send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
            char *fs_name = "sentfromclient.jpg";
            char sendbuff[LENGTH];
            printf("Client send %s to the Server...", fs_name);
            FILE *fs = fopen(fs_name, "r");
            if (fs == NULL) {
                printf("Error: Dont find file...\n");
                exit(1);
            }
            bzero(sendbuff, LENGTH);
            int fs_block_sz;
            while((fs_block_sz = fread(sendbuff, sizeof(char), LENGTH, fs))> 0)
            {
                if (send(socketfd, sendbuff,fs_block_sz, 0) <0) {
                    fprintf(stderr, "ERROR: fail to send file");
                    break;                            
                }
                bzero(sendbuff, LENGTH);
            }
            printf("File was successfully sent!\n");  */
            transfer(&user,&option[9]);
        }
        else if (!strncmp(option, "flag", 4)) {
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected...\n");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "flag");
            strcpy(package.name, user.name);
            strcpy(package.buff, "msg");
            package.clientnum = user.clientnum;
            send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
        }
        else if(!strncmp(option, "quit", 4)) {
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected...\n");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "quit");
            strcpy(package.name, user.name);
            strcpy(package.buff, "msg");
            package.clientnum = user.clientnum;
            send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
        }
        else if(!strncmp(option, "broadcast", 9)) {
            broadcast(&user,&option[10]);
        }
        else if(!strncmp(option, "logout", 6)) {
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected... \n");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "exit");
            strcpy(package.name, user.name);
            sent = send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
            isconnected = 0;
        }
        else fprintf(stderr, "Unknown option...\n");
    }
    return 0;
}

int connectserver() {
    int newfd, err_ret;
    struct sockaddr_in serv_addr;
    struct hostent *to;
    if ((to = gethostbyname(IP))==NULL) {
        err_ret = errno;
        fprintf(stderr, "gethostbyname() errno...\n");
        return err_ret;
    }
    if ((newfd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        err_ret = errno;
        fprintf(stderr, "socket() errno");
        return err_ret;
    }
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)to->h_addr);
    memset(&(serv_addr.sin_zero), 0, 8);
    if (connect(newfd,(struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
        err_ret = errno;
        fprintf(stderr, "connect() error\n");
        return err_ret;
    }
    else {
        fprintf("Connect to server at %s: %d\n",IP, PORT);
        return newfd;
    }
}

void setname(struct USER *user) {
    int sent;
    struct PACKAGE package;
    if (!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    memset(&package, 0, sizeof(struct PACKAGE));
    strcpy(package.option, "name");
    strcpy(package.name, user->name);
    sent = send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
}

void *receiver(void *param) {
    int recvd;
    struct PACKAGE package;
    printf("Waiting here [%d]...\n", socketfd);
    while (isconnected) {
        recvd = recv(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
        if (!recvd) {
            fprintf(stderr, "Connection lost...\n");
            isconnected = 0;
            close(socketfd);
            break;
        }
        if (recvd > 0) {
            printf("[%s]: %s\n", package.name, package.buff);
            user.clientnum = package.clientnum;
        }
        memset(&package, 0, sizeof(struct PACKAGE));
    }
    return NULL;
}

void *fileReceiver(void *param) {
    int recvd;
    char recvbuff[LENGTH];
    struct PACKAGE package;
    printf("Waiting [%d]...\n", socketfd);
    while (isconnected) {
        char* fr_name = "receive.jpg";
        FILE *fr = fopen(fr_name, "w");
        //print ("aaa");
        if (fr == NULL) {
            printf("File does not exist, create a new one", fr_name);
            fr = fopen("receivetoclient.jpg","w+");
        }
        bzero(recvbuff,LENGTH);
        int fr_block_sz =0;
        while ((fr_block_sz = recv(socketfd,recvbuff,LENGTH,0)) >0) {
            int write_sz = fwrite(recvbuff, sizeof(char), fr_block_sz, fr);
            if (write_sz< fr_block_sz) {
                error ("Fail to write file...\n");
            }
            bzero(recvbuff, LENGTH);
            if (fr_block_sz ==0 ||fr_block_sz != 512) {
                break;
            }
            if (fr_block_sz < 0) {
                if (errno == EAGAIN) {
                    printf("recv() time out...\n");
                }
                else {
                    fprintf(stderr, "recv() failed due to errno = %d\n", errno);
                }
            }
            printf("Successfully received file...\n");
        }
        fclose(fr);
    }
    printf("errno2: %s\n", strerror(errno));
    return NULL;
}

void broadcast(struct USER *user, char *msg) {
    int sent;
    struct PACKAGE package;
    if (msg == NULL) {
        return;
    }
    if (!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    msg[BUFFLEN] = 0;
    memset(&package, 0, sizeof (struct PACKAGE));
    strcpy(package.option, "broadcast");
    strcpy(package.name, user->name);
    strcpy(package.buff, msg);
    package.clientnum = user->clientnum;
    sent = send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
}

void whisp(struct USER *user, char *msg) {
    int sent;
    struct PACKAGE package;
    if (msg == NULL) {
        return;
    }
    if (!isconnected) {
        fprintf(stderr, "You are not connected...\n");
        return;
    }
    msg[BUFFLEN] = 0;
    memset(&package, 0, sizeof (struct PACKAGE));
    strcpy(package.option, "whisp");
    strcpy(package.name, user->name);
    strcpy(package.buff, msg);
    package.clientnum = user->clientnum;
    sent = send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
} 

void transfer(struct USER *user, char *msg) {
            struct PACKAGE package;
            if (!isconnected) {
                fprintf(stderr, "You are not connected");
                return;
            }
            memset(&package, 0, sizeof(struct PACKAGE));
            strcpy(package.option, "transfer");
            strcpy(package.name, user->name);
            strcpy(package.buff, msg);
            package.clientnum = user->clientnum;
            send(socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
            char *fs_name = msg;
            char sendbuff[LENGTH];
            printf("Client send %s to the Server...", fs_name);
            FILE *fs = fopen(fs_name, "r");
            if (fs == NULL) {
                printf("Error: Dont find file...\n");
                exit(1);
            }
            bzero(sendbuff, LENGTH);
            int fs_block_sz;
            while((fs_block_sz = fread(sendbuff, sizeof(char), LENGTH, fs))> 0)
            {
                if (send(socketfd, sendbuff,fs_block_sz, 0) <0) {
                    fprintf(stderr, "ERROR: fail to send file");
                    break;                            
                }
                bzero(sendbuff, LENGTH);
            }
            printf("File was successfully sent!\n");

}
