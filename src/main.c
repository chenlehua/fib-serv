#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include "../libs/helpers.h"
#include "../libs/structs.h"
#include "../libs/macros.h"

//global variables

atomic_int threadCounter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void renewThread(void *arg) {
    int *acceptedSocket = (int *) arg;
    close(*acceptedSocket);
    pthread_mutex_lock(&mutex);
    threadCounter--;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}


noreturn void *acceptConn(void *arg) {
    acceptParams *ap = (acceptParams *) arg;
    int acceptedSocket;

    while (1) {
        pthread_cleanup_push(renewThread, &acceptedSocket) ;

                if ((acceptedSocket = accept(ap->serverFd, ap->addr, ap->addrLen)) < 0) {
                    perror("In accept");
                    pthread_exit(NULL);
                }

                char reqBuf[HTTP_REQ_BUF];
                bzero(reqBuf, HTTP_REQ_BUF);
                const size_t receiveBytes = read(acceptedSocket, reqBuf, HTTP_REQ_BUF);
                if (receiveBytes > 0) {
                    char resBuf[HTTP_RES_BUF];

                    pthread_mutex_lock(&mutex);
                    const int num = retrieveGETQueryIntValByKey(reqBuf, "num");
                    pthread_mutex_unlock(&mutex);

                    int fibResult = calcFibonacci(num);

                    sprintf(resBuf, "HTTP/1.1 200 OK\r\n"
                                    "Content-type: text/plain\r\n"
                                    "Content-length: %d\r\n\n%d", calcDigits(fibResult), fibResult);
                    write(acceptedSocket, resBuf, strlen(resBuf));
                }


                close(acceptedSocket);
        pthread_cleanup_pop(0);
    }
}

int main(int argc, const char *argv[]) {
    // initialize the server setup.
    serverSettings ss = {.threadCount=4};
    setupServerSettings(argc, argv, &ss);

    int serverFd;
    sockaddr_in address;
    int addrLen = sizeof(address);

    //establish a socket
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("In socket creation");
        exit(EXIT_FAILURE);
    }

    bzero(&address, addrLen);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (sockaddr *) &address, sizeof(address)) < 0) {
        perror("In bind");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, MAX_LISTEN_CONN) < 0) {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    printf("\n Server is now listening at port %d:\n\n", PORT);

    //main loop.
    while (1) {
        pthread_mutex_lock(&mutex);
        while (threadCounter >= ss.threadCount)
            pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);

        pthread_t thread_id;
        acceptParams ap = {serverFd, (sockaddr *) &address, (socklen_t *) &addrLen};
        pthread_create(&thread_id, NULL, acceptConn, &ap);
        atomic_fetch_add(&threadCounter, 1);
        printf("[Info] Thread Created: No.%d\n", threadCounter);

    }


    printf("sfsf");
    return EXIT_SUCCESS;
}