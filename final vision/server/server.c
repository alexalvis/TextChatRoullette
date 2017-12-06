#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<netdb.h>
#include"server.h"

int socketfd, newfd;
struct THREADINFO  threadinfo[CLIENTNUM];   //thread_info
struct LLIST clientlist;
pthread_mutex_t clientlist_mutex;
int clientNUM = 1;
int chatpairNUM= 0;
int flag[CLIENTNUM]={0};

void *io_handler(void *param);
void *client_handler(void *filedes);
void block(char *ptr);
void unblock(char *ptr);
void throwout(char *ptr);

int compare(struct THREADINFO *a, struct THREADINFO *b) {
    return a->socketfd - b->socketfd;
};

void listinitial(struct LLIST *ll) {
    ll->head = ll->tail = NULL;
    ll->size = 0;
};

int listinsert (struct LLIST *ll, struct THREADINFO *thr_info) {
    if(ll->size == CLIENTNUM) return -1;
    if(ll->head == NULL) {
        ll->head = (struct LNODE *)malloc(sizeof(struct LNODE));
        ll->head->threadinfo = *thr_info;
        ll->head->next = NULL;
        ll->tail =ll->head;
    }
    else {
        ll->tail->next = (struct LNODE *)malloc(sizeof(struct LNODE));
        ll->tail->next->threadinfo = *thr_info;
        ll->tail->next->next = NULL;
        ll->tail=ll->tail->next;
    }
    ll->size++;
    return 0;
}

int listdelete (struct LLIST *ll, struct THREADINFO *thr_info) {
    if (ll->head == NULL) return -1;
    struct LNODE *curr, *temp;
    if (compare(thr_info, &ll->head->threadinfo) == 0) {
        temp = ll->head;
        ll->head = ll->head->next;
        if (ll->head == NULL) ll->tail = ll->head;
        free(temp);
        ll->size--;
        return 0;
    }
    for(curr= ll->head; curr->next != NULL; curr = curr->next) {
        if (compare(thr_info, &curr->next->threadinfo) == 0) {
            temp = curr->next;
            if (temp == ll->tail)
                ll->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            ll->size--;
            return 0;
        }
    }
    return -1;
}

void listtraversal(struct LLIST *ll) {
    struct LNODE *curr;
    struct THREADINFO *thr_info;
    int chatnum = 0;
    for (curr = clientlist.head; curr != NULL; curr=curr->next) {
        if (curr->threadinfo. chatnum > 0) {
            chatnum++;
        }
    }
    printf("----------------------------------------------------\n");
    printf("Number of clients in the queue: %d\n", ll->size);
    printf("Number of chatting now: %d\n", chatnum);
    printf("socketfd name clientnum chatnum flagtime\n");
    for (curr = ll->head; curr !=NULL; curr= curr->next) {
        thr_info = &curr->threadinfo;
        printf("[%d]    %s    %d         %d        %d\n", thr_info->socketfd, 
                thr_info->name, thr_info->clientnum, thr_info->chatnum, flag[thr_info->clientnum - 1]);
    }
    printf("----------------------------------------------------\n");
}

int main(int argc, char **argv) {
    int err_ret, sin_size;
    struct sockaddr_in serv_addr, client_addr;
    pthread_t interrupt;
    char option[LINEBUFF];
    while(gets(option)) {
        if(!strncmp(option, "start", 5)) {

            listinitial(&clientlist);    //initial link list

            pthread_mutex_init(&clientlist_mutex, NULL);    //initial mutex

            if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                err_ret =errno;
                fprintf(stderr, "socket() error...\n");
                return err_ret;
            }
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(PORT);
            serv_addr.sin_addr.s_addr = inet_addr(IP);
            memset(&(serv_addr.sin_zero), 0, 8);             //create socket

            if (bind(socketfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
                err_ret = errno;
                fprintf(stderr, "bind() failed... \n");
                return err_ret;
            }                                                //bind address with socket

            if(listen(socketfd,BACKLOG) == -1) {
                err_ret = errno;
                fprintf(stderr, "pthread_create() failed... \n");
                return err_ret;
            }                                                // start listening for connection
            
            printf("Starting admin interface...\n");
            if(pthread_create(&interrupt, NULL, io_handler, NULL) != 0) {
                err_ret = errno;
                fprintf(stderr, "pthread_create() failed...\n");
                return err_ret;
            }                                                 //initiate interrupt handler for IO controlling

            printf("Start socket listening...\n");
            while(1) {
                sin_size = sizeof(struct sockaddr_in);
                if((newfd = accept(socketfd,(struct sockaddr *)&client_addr, (socklen_t*)&sin_size)) == -1) {
                    err_ret = errno;
                    fprintf(stderr, "accept failed...\n");
                    return err_ret;
                }
                else {
                    if(clientlist.size == CLIENTNUM) {
                        printf("list is full, reject connection...\n");
                        struct PACKAGE client_package;
                        memset(&client_package, 0, sizeof(struct PACKAGE));
                        strcpy(client_package.option, "msg");
                        strcpy(client_package.buff, "list is full, your requirement has been rejected.");
                        send(newfd,(void *)&client_package, sizeof(struct PACKAGE), 0);
                        continue;
                    }
                    printf("connection request got...\n");
                    struct THREADINFO threadinfo;
                    threadinfo.socketfd = newfd;
                    strcpy(threadinfo.name, "Anonymous");
                    threadinfo.clientnum=clientNUM++;
                    threadinfo.chatnum=0;
                    pthread_mutex_lock(&clientlist_mutex);
                    listinsert(&clientlist, &threadinfo);
                    pthread_mutex_unlock(&clientlist_mutex);
                    pthread_create(&threadinfo.thread_num, NULL, client_handler, (void *)&threadinfo);
                    struct PACKAGE package;
                    memset(&package, 0, sizeof(struct PACKAGE));
                    package.clientnum=threadinfo.clientnum;
                    send(threadinfo.socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
                }
            }
            return 0;
        }
        else {
            fprintf(stderr, "you need to start a server first.\n");
        }
    }
}                                               //keep connecting

void *io_handler(void *param) {
    char option[LINEBUFF];
    int targetfd;
    struct LNODE *curr;

    while(gets(option)) {
        if(!strncmp(option, "exit", 4)) {
            printf("exit server...\n");
            pthread_mutex_destroy(&clientlist_mutex);
            close(socketfd);
            exit(0);
        }
        else if (!strncmp(option, "end", 3)) {
            printf("SERVER has been closed.\n");
            pthread_mutex_lock(&clientlist_mutex);
            for(curr=clientlist.head; curr != NULL; curr=curr->next) {
                if(curr->threadinfo.chatnum <= 0) {
                    struct PACKAGE temppackage;
                    memset(&temppackage, 0, sizeof(struct PACKAGE));
                    strcpy(temppackage.option, "msg");
                    strcpy(temppackage.name, curr->threadinfo.name);
                    strcpy(temppackage.buff, "SERVER has been closed.");
                    send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
                    listdelete(&clientlist, &(curr->threadinfo));
                }
                else {
                    struct PACKAGE temppackage;
                    memset(&temppackage, 0, sizeof(struct PACKAGE));
                    strcpy(temppackage.option, "msg");
                    strcpy(temppackage.name, curr->threadinfo.name);
                    strcpy(temppackage.buff, "SERVER has been closed. Your chatting has been shut down");
                    send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }                   // option end the chat server and inform the users 
        
        else if(!strncmp(option, "stats", 5)) {
            pthread_mutex_lock(&clientlist_mutex);
            listtraversal(&clientlist);
            pthread_mutex_unlock(&clientlist_mutex);
        }
                    
        else if(!strncmp(option, "block", 5)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            block(ptr);
        }

        else if(!strncmp(option, "unblock", 7)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            unblock(ptr);
        }

        else if(!strncmp(option, "throwout", 8)) {
            char *ptr = strtok(option, " ");
            ptr = strtok(0, " ");
            throwout(ptr);
        }
        else {
            fprintf(stderr, "unknown command: %s\n", option);
        }
    }
    return NULL;
}

void *client_handler(void *fd) {
    struct THREADINFO threadinfo = *(struct THREADINFO *)fd;
    struct PACKAGE package;
    struct LNODE *curr;
    int bytes, sent;

    BOOL flagbusy = TRUE;   //flagbusy is TRUE means no client available.false means at least one waiting

    while(1) {
        bytes = recv(threadinfo.socketfd, (void *)&package, sizeof(struct PACKAGE), 0);
        if(!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", threadinfo.socketfd, threadinfo.name);
            pthread_mutex_lock(&clientlist_mutex);
            listdelete(&clientlist, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }

        if (!strcmp(package.option, "name")) {
            printf("Set name to %s\n", package.name);
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = clientlist.head; curr != NULL; curr = curr->next) {
                if (compare(&curr->threadinfo, &threadinfo) == 0) {
                    strcpy(curr->threadinfo.name, package.name);
                    strcpy(threadinfo.name, package.name);
                    break;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }

        else if(!strcmp(package.option, "chat")) {
            struct THREADINFO tempthreadinfo;
            pthread_mutex_lock(&clientlist_mutex);
            for (curr = clientlist.head; curr!= NULL; curr = curr->next) {
                if(curr->threadinfo.socketfd == threadinfo.socketfd) {
                    tempthreadinfo = curr->threadinfo;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        
            if(tempthreadinfo.chatnum == -1) {
                printf("SERVER: You have been blocked.\n");
            }                                //do nothing
        
            else if(tempthreadinfo.chatnum > 0) {
                printf("SERVER: You have a partner.\n");
                struct PACKAGE temppackage;
                memset(&temppackage, 0, sizeof(struct PACKAGE));
                strcpy(temppackage.option, "msg");
                strcpy(temppackage.name, package.name);
                temppackage.clientnum = tempthreadinfo.clientnum;
                strcpy(temppackage.buff, "CLIENT: You have a partner.");
                send(tempthreadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
            }                              //the client already have a partner
            else {
                pthread_mutex_lock(&clientlist_mutex);
                for(curr = clientlist.head; curr !=NULL; curr= curr->next) {
                    if (curr->threadinfo.chatnum == 0) {
                        if (!compare(&curr->threadinfo, &tempthreadinfo)) continue;
                        flagbusy = FALSE;   //flagbusy =FLASE means there is client available
                    } 
                }
                pthread_mutex_unlock(&clientlist_mutex);
                if (flagbusy == TRUE) {      //flagbusy = TRUE means there is no client available 
                    printf("SERVER: No client available now\n");
                    struct PACKAGE temppackage;
                    memset(&temppackage, 0, sizeof(struct PACKAGE));
                    strcpy(temppackage.option, "msg");
                    strcpy(temppackage.name, tempthreadinfo.name);
                    temppackage.clientnum = tempthreadinfo.clientnum;
                    strcpy(temppackage.buff, "CLIENT: No Client Available Now.");
                    sent= send(tempthreadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
                }
                else {
                    chatpairNUM++;      
                    pthread_mutex_lock(&clientlist_mutex);
                    for(curr = clientlist.head; curr != NULL; curr = curr->next) {
                        if (compare(&curr->threadinfo, &tempthreadinfo) == 0) {         
                            curr->threadinfo.chatnum = chatpairNUM;
                            tempthreadinfo.chatnum = chatpairNUM;
                        }
                    }         //set the client's chatnum to chatpairnum
                    pthread_mutex_unlock(&clientlist_mutex);
                    pthread_mutex_lock(&clientlist_mutex);
                    for (curr = clientlist.head; curr != NULL; curr = curr->next) {
                        if (curr->threadinfo.chatnum == 0) {
                            if (!compare(&curr->threadinfo, &tempthreadinfo)) continue;
                            curr->threadinfo.chatnum = chatpairNUM;
                            printf("SERVER: Find a Partner: \n");
                            printf("SERVER: my clientnum: %d, my name: %s, my chatnum: %d\n",  tempthreadinfo.clientnum, tempthreadinfo.name, tempthreadinfo.chatnum);
                            printf("SERVER: my partner clientnum: %d, my partner name: %s, my partner chatnum: %d\n",  curr->threadinfo.clientnum, curr->threadinfo.name, curr->threadinfo.chatnum);   
                            struct PACKAGE temppackage_server;
                            memset(&temppackage_server, 0, sizeof(struct PACKAGE));
                            strcpy(temppackage_server.option, "msg");
                            strcpy(temppackage_server.name, curr->threadinfo.name);
                            char partnername[Name_Length];
                            strcpy(partnername, tempthreadinfo.name);
                            char partnerbuff[LINEBUFF];
                            strcat(partnerbuff, "You have got a partner: ");
                            strcat(partnerbuff, partnername);
                            strcpy(temppackage_server.buff, partnerbuff);
                            temppackage_server.clientnum = curr->threadinfo.clientnum;
                            sent = send(curr->threadinfo.socketfd, (void *)&temppackage_server, sizeof(struct PACKAGE), 0);   
                            struct PACKAGE temppackage_client;
                            memset(&temppackage_client, 0,  sizeof(struct PACKAGE));
                            strcpy(temppackage_client.option, "msg");
                            temppackage_client.clientnum = curr->threadinfo.clientnum;
                            strcpy(temppackage_client.name, curr->threadinfo.name);
                            char myname[Name_Length];
                            strcpy(myname, curr->threadinfo.name);
                            char mybuff[LINEBUFF];
                            strcat(mybuff, "You have got a partner: ");
                            strcat(mybuff, myname);
                            strcpy(temppackage_client.buff, mybuff);
                            sent = send(tempthreadinfo.socketfd, (void *)&temppackage_client, sizeof(struct PACKAGE), 0);
                            break; 
                        }
                    }
                    pthread_mutex_unlock(&clientlist_mutex);
                }
            }
        }

        else if(!strcmp(package.option, "whisp")) {
            struct THREADINFO threadinfo_whisp;
            pthread_mutex_lock(&clientlist_mutex);
            for(curr= clientlist.head; curr!= NULL; curr=curr->next) {
                if (curr->threadinfo.socketfd == threadinfo.socketfd) {
                    threadinfo_whisp = curr->threadinfo;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
            if(threadinfo_whisp.chatnum == -1) {
                printf("SERVER: You are blocked.\n");
            }
            else if(threadinfo_whisp.chatnum == 0) {
                printf("You need to type \" chat \" to get a partner.");
            }
            else {
                pthread_mutex_lock(&clientlist_mutex);
                for (curr = clientlist.head; curr!=NULL; curr=curr->next) {
                    if(threadinfo_whisp.chatnum == curr->threadinfo.chatnum) {
                        if (!compare(&curr->threadinfo,&threadinfo_whisp)) continue;
                        printf("SERVER: Whisp to client NO. %d\n", curr->threadinfo.clientnum);
                        struct PACKAGE temppackage;
                        memset(&temppackage, 0, sizeof(struct PACKAGE));
                        strcpy(temppackage.option, "msg");
                        strcpy(temppackage.name, package.name);
                        strcpy(temppackage.buff, package.buff);
                        temppackage.clientnum = threadinfo_whisp.clientnum;
                        send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
                    }
                }
                pthread_mutex_unlock(&clientlist_mutex);
            }
        }

        else if(!strcmp(package.option, "flag")) {
            struct THREADINFO threadinfo_flag;
            pthread_mutex_lock(&clientlist_mutex);
            for (curr=clientlist.head; curr!= NULL; curr=curr->next) {
                if (curr->threadinfo.socketfd == threadinfo.socketfd) {
                    threadinfo_flag = curr->threadinfo;
                }            
            }
            pthread_mutex_unlock(&clientlist_mutex);
            if(threadinfo_flag.chatnum == -1) {
                printf("SERVER: You are Blocked.\n");
            }
            else if (threadinfo_flag.chatnum == 0) {
                printf("You need to type \"chat \" to get a partner.\n");
                struct PACKAGE temppackage;
                memset(&temppackage, 0, sizeof(struct PACKAGE));
                strcpy(temppackage.option,"msg");
                strcpy(temppackage.name,threadinfo_flag.name);
                strcpy(temppackage.buff,"Client: You need to type \" chat\" to get a partner.");
                temppackage.clientnum = threadinfo_flag.clientnum;
                send(threadinfo_flag.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0); 
            }
            else {
                pthread_mutex_lock(&clientlist_mutex);
                for(curr = clientlist.head; curr!=NULL; curr=curr->next) {
                    if (curr->threadinfo.chatnum == threadinfo_flag.chatnum) {
                        if(!compare(&curr->threadinfo, &threadinfo_flag)) continue;
                        flag [curr->threadinfo.clientnum - 1]++;
                        printf("Server: Client No. %d is flagged %d times.\n", curr->threadinfo.clientnum, flag[curr->threadinfo.clientnum -1]);
                        struct PACKAGE temppackage;
                        memset(&temppackage, 0, sizeof (struct PACKAGE));
                        strcpy(temppackage.option, "msg");
                        strcpy(temppackage.name,curr->threadinfo.name);
                        strcpy(temppackage.buff, "Client: Your Partner Report You.");
                        temppackage.clientnum = threadinfo_flag.clientnum;
                        send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof (struct PACKAGE), 0);
                        break;
                    }
                }
                pthread_mutex_unlock(&clientlist_mutex);
            }
        }

        else if(!strcmp(package.option, "transfer")) {
            char recfilebuff[512];
            char* recfile_name = package.buff;
            FILE *recfile = fopen(recfile_name, "a");
            if (recfile == NULL) {
                printf("file is not existing, create a new one.",recfile_name);
                recfile = fopen(recfile_name , "w+");
            }
            bzero(recfilebuff, LENGTH);
            int recfile_block = 0;
            while((recfile_block = recv(newfd, recfilebuff, LENGTH,0)) > 0) {
                int writefile = fwrite(recfilebuff, sizeof(char), recfile_block, recfile);
                if (writefile < recfile_block) {
                    error("SERVER: File write fails.\n");
                }
                bzero(recfilebuff, LENGTH);
                if (recfile_block == 0 || recfile_block!=512) {
                    break;
                }
            }
            if (recfile_block<0) {
                if (errno == EAGAIN) {
                    printf("recv() time out. \n");
                }
                else {
                    fprintf(stderr, "recv() failed due to errno = %d\n", errno);
                    exit(1);
                }
            }
            printf("Received File from client.\n");
            fclose(recfile);
        }
        else if(!strcmp(package.option, "quit")) {
            struct THREADINFO threadinfo_quit;
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = clientlist.head; curr!= NULL; curr= curr->next) {
                if (curr->threadinfo.socketfd == threadinfo.socketfd) {
                    threadinfo_quit = curr->threadinfo;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
            if (threadinfo_quit.chatnum == -1) {
                printf("SERVER: You are blocked.\n");
            }
            else if (threadinfo_quit.chatnum == 0) {
                printf("SERVER: You need to type \" chat\" to get a partner.");
            }
            else {
                pthread_mutex_lock(&clientlist_mutex);
                for (curr= clientlist.head; curr!=NULL; curr= curr->next) {
                    if(curr->threadinfo.chatnum == threadinfo_quit.chatnum) {
                        if (!compare(&curr->threadinfo, &threadinfo_quit)) continue;
                        curr->threadinfo.chatnum = 0;
                        struct PACKAGE temppackage;
                        memset(&temppackage, 0, sizeof(struct PACKAGE));
                        strcpy(temppackage.option,"msg");
                        strcpy(temppackage.name,curr->threadinfo.name);
                        strcpy(temppackage.buff, "Client: Your chatter quit the chat.");
                        temppackage.clientnum = curr->threadinfo.clientnum;
                        send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
                    }
                }
                pthread_mutex_unlock(&clientlist_mutex);
                pthread_mutex_lock(&clientlist_mutex);
                for (curr = clientlist.head; curr!=NULL; curr= curr->next) {
                    if (compare(&curr->threadinfo, &threadinfo_quit) == 0) {
                        curr->threadinfo.chatnum = 0;
                        threadinfo.chatnum = 0;
                    }
                }
                pthread_mutex_unlock(&clientlist_mutex);
            }
        }
  
        else if(!strcmp(package.option, "broadcast")) {
            pthread_mutex_lock(&clientlist_mutex);
            for (curr = clientlist.head; curr!=NULL; curr=curr->next) {
                struct PACKAGE temppackage;
                memset(&temppackage, 0, sizeof(struct PACKAGE));
                if (!compare(&curr->threadinfo, &threadinfo)) continue;
                strcpy(temppackage.option, "msg");
                strcpy(temppackage.name, package.name);
                strcpy(temppackage.buff, package.buff);
                send(curr->threadinfo.socketfd, (void *)&temppackage, sizeof(struct PACKAGE), 0);
            
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }

        else if(!strcmp(package.option, "exit")) {
            printf("SERVER: [%d] %s has disconnected...\n", threadinfo.socketfd, threadinfo.name);
            pthread_mutex_lock(&clientlist_mutex);
            listdelete(&clientlist, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }
        else {
            fprintf(stderr, "SERVER: Garbage data from [%d] %s...\n", threadinfo.socketfd, threadinfo.name);
        }
    }
    close(threadinfo.socketfd);
    return NULL;
}

void block(char *ptr) {
    struct LNODE *curr;
    struct THREADINFO threadinfo_block;
    BOOL flagID = FALSE;    // True means it is legal,False means it is illegal
    int i;
    if (ptr == NULL) {
        return;
    }
    i = atoi(ptr);
    pthread_mutex_lock(&clientlist_mutex);
    for(curr = clientlist.head; curr!= NULL; curr= curr->next) {
        if (curr->threadinfo.clientnum == i) {
            flagID = TRUE;
            threadinfo_block = curr->threadinfo;
        }
    }
    pthread_mutex_unlock(&clientlist_mutex);
    if (flagID == FALSE) {
        printf("SERVER: Can not find this client: %d\n", i);
        return;
    }              //client is illegal
    if (threadinfo_block.chatnum == -1) {
        printf("This client is already blocked.\n");
        return;
    }
    if (threadinfo_block.chatnum == 0) {
        pthread_mutex_lock(&clientlist_mutex);
        for(curr = clientlist.head; curr!=NULL; curr=curr->next) { 
                if (!compare(&curr->threadinfo, &threadinfo_block)) {
                    curr->threadinfo.chatnum = -1;
                    threadinfo_block.chatnum = -1;
                    printf("Successfully block the client...\n");
                    struct PACKAGE package_block;
                    memset(&package_block, 0, sizeof (struct PACKAGE));
                    strcpy(package_block.option, "msg");
                    strcpy(package_block.name, curr->threadinfo.name);
                    strcpy(package_block.buff, "Client: You are blocked.");
                    send(curr->threadinfo.socketfd, (void *)&package_block,  sizeof(struct PACKAGE), 0);
                }
        }
        pthread_mutex_unlock(&clientlist_mutex);
    }
    else { 
        pthread_mutex_lock(&clientlist_mutex);
        for (curr = clientlist.head; curr!=NULL; curr= curr->next) {
            if (curr->threadinfo.chatnum ==threadinfo_block.chatnum) {
                if (!compare(&curr->threadinfo, &threadinfo_block)) continue;
                curr->threadinfo.chatnum =0;
                struct PACKAGE package_block;
                memset(&package_block, 0, sizeof(struct PACKAGE));
                strcpy(package_block.option, "msg");
                strcpy(package_block.name, curr->threadinfo.name);
                strcpy(package_block.buff, "Client: Your partner has been blocked, your chat is end.");
                send(curr->threadinfo.socketfd, (void *)&package_block, sizeof(struct PACKAGE), 0);
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
        pthread_mutex_lock(&clientlist_mutex);
        for (curr=clientlist.head; curr!=NULL; curr= curr->next) {
            if (!compare(&curr->threadinfo, &threadinfo_block)) {
                curr->threadinfo.chatnum = -1;
                threadinfo_block.chatnum = -1;
                struct PACKAGE package_block;
                memset(&package_block, 0, sizeof (struct PACKAGE));
                strcpy(package_block.option, "msg");
                strcpy(package_block.name, curr->threadinfo.name);
                strcpy(package_block.buff, "Client: You have been blocked.");
                send(curr->threadinfo.socketfd, (void *)&package_block, sizeof(struct PACKAGE), 0);
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
        printf("SERVER: This Client has been successfully blocked. Shut Down the Channel...\n");
    }
}

void unblock(char *ptr) {
    struct LNODE *curr;
    struct THREADINFO threadinfo_unblock;
    BOOL flagID = FALSE;
    int i;
    if (ptr == NULL) {
        return;
    }
    i = atoi(ptr);
    pthread_mutex_lock(&clientlist_mutex);
    for(curr = clientlist.head; curr!= NULL; curr= curr->next) {
        if (curr->threadinfo.clientnum == i) {
            flagID = TRUE;
            threadinfo_unblock = curr->threadinfo;
        }
    }
    pthread_mutex_unlock(&clientlist_mutex);
    if (flagID == FALSE) {
        printf("Can not find this client: %d\n", i);
        return;
    }
    if (threadinfo_unblock.chatnum == -1) {
        pthread_mutex_lock(&clientlist_mutex);
        for (curr=clientlist.head; curr!= NULL; curr= curr->next) {
            if (!compare(&curr->threadinfo,&threadinfo_unblock)) {
                curr->threadinfo.chatnum = 0;
                threadinfo_unblock.chatnum = 0;
                printf("SERVER: This client is now unblocked...\n");
                struct PACKAGE package_unblock;
                memset (&package_unblock, 0, sizeof (struct PACKAGE));
                strcpy(package_unblock.option,"msg");
                strcpy(package_unblock.name, curr->threadinfo.name);
                strcpy(package_unblock.buff, "Client: You are now unblocked.");
                send(curr->threadinfo.socketfd, (void *)&package_unblock, sizeof (struct PACKAGE), 0);
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
    }
    else {
        printf("The client is not blocked...\n");
        return;
    }
}

void throwout(char *ptr) {
    struct LNODE *curr;
    struct THREADINFO threadinfo_throwout;
    BOOL flagID= FALSE;
    int i;
    if(ptr == NULL) {
        return;
    }
    i = atoi(ptr);
    pthread_mutex_lock(&clientlist_mutex);
    for (curr = clientlist.head; curr !=NULL; curr=curr->next) {
        if (curr->threadinfo.clientnum == i) {
            flagID = TRUE;
            threadinfo_throwout = curr->threadinfo;
        }
    }
    pthread_mutex_unlock(&clientlist_mutex);
    if (flagID== FALSE) {
        printf("SERVER: Can't Find the Client");
        return;
    }
    if (threadinfo_throwout.chatnum == -1||threadinfo_throwout.chatnum == 0) {
        pthread_mutex_lock(&clientlist_mutex);
        for (curr=clientlist.head; curr!=NULL; curr= curr->next) {
            if (curr->threadinfo.clientnum == i) {
                listdelete(&clientlist,&(curr->threadinfo));
                printf("SERVER: Client %d is Thrown Out.\n", i);
                break;
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
    }
    else {
        pthread_mutex_lock(&clientlist_mutex);
        for (curr= clientlist.head; curr!=NULL; curr = curr->next) {
            if(curr->threadinfo.chatnum ==threadinfo_throwout.chatnum) {
                if(!compare(&curr->threadinfo, &threadinfo_throwout)) continue;
                curr->threadinfo.chatnum = 0;
                struct PACKAGE package_throwout;
                memset(&package_throwout, 0, sizeof(struct PACKAGE));
                strcpy(package_throwout.option, "msg");
                strcpy(package_throwout.name, curr->threadinfo.name);
                strcpy(package_throwout.buff, "Client: Your partner has been thrown out. Your chat is end.");
                send(curr->threadinfo.socketfd, (void *)&package_throwout, sizeof(struct PACKAGE), 0);
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
        pthread_mutex_lock(&clientlist_mutex);
        for (curr= clientlist.head; curr!=NULL; curr = curr->next) {
            if(!compare(&curr->threadinfo, &threadinfo_throwout)) {
                curr->threadinfo.chatnum = 0;
                threadinfo_throwout.chatnum = 0;
                struct PACKAGE package_throwout;
                memset(&package_throwout, 0, sizeof(struct PACKAGE));
                strcpy(package_throwout.option, "msg");
                strcpy(package_throwout.name, curr->threadinfo.name);
                strcpy(package_throwout.buff, "Client: You have been thrown out. Your chat is end.");
                send(curr->threadinfo.socketfd, (void *)&package_throwout, sizeof(struct PACKAGE), 0);
            }
        }
        pthread_mutex_unlock(&clientlist_mutex);
    }

}

