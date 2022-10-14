
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <unistd.h>


#define MAXLINE 9 //max text line
#define SERV_PORT 10020 
#define LISTENQ 8
#define SEND_LENGTH 14
uint32_t a, b, result;
char c;

/********** Method Declarations **********/
void make_packet(uint32_t a, uint32_t b, uint32_t result, char c, char *m);
void unmake_packet(char *recvM);
uint32_t calc (uint32_t a, uint32_t b , char c);

int main ( int argc, char **argv)
{
    int listenfd, connfd, n;
    pid_t childpid;
    socklen_t clilen;
    char buf[9];
    char M[12];
    struct sockaddr_in cliaddr, servaddr;
    //char *array [4];
    
    //Create a socket for the server
    //If sockfd<0 there was an error in the creation of the socket
    if((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0){
    perror("Problem in creating the socket");
    exit(2);
    }
    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    //bind the socket
    bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr));
    //listen to the socket by creating a connection queue, then wait for clients 
    listen (listenfd, LISTENQ);
    printf("%s \n","Server running...waiting for connections.");
    for( ; ; ){
        clilen = sizeof (cliaddr);
        //accept a connection
        connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);//blocked, wait for connection
        printf("%s \n","Received request");
        if((childpid = fork ()) == 0 ) {//if it is 0, it is the child process
        printf ("%s \n","Child created for dealing with client request");
        //close listening socket
        close (listenfd);
        while((n = recv(connfd, buf, MAXLINE,0)) > 0)
        {
            //send(sockfd, result, SEND_LENGTH, 0);

            printf("% s","Msg received from the client: ");
            unmake_packet(buf);
            //puts(result);
            printf("the number a is %d", a);
            printf("the number b is %d", b);
            printf("the operator c is %c", c);
            make_packet(a,b, result, c, M);
            send(connfd, M, SEND_LENGTH, 0);
        }
        if(n <0)
            printf("%s \n", "Read error");
         exit(0);// child exit
        }
        //close socket of the server
        close(connfd);
    }
}    

/********** Method Definitions **********/
void make_packet(uint32_t a, uint32_t b, uint32_t res, char c, char *m) {
    int i;
    char opC[1];
    char opA[4];
    char opB[4];
    char answer[4];

    for (i = 0; i < 4; i++) {
        opA[i] = (a >> (i * 8)) & 0xFF;
        opB[i] = (b >> (i * 8)) & 0xFF;
        answer[i] = (res >> ( i *8)) & 0xFF;
    }
    
    // Just put c in the temp array to be placed in the packet
    opC[0] = c;

    // Put eveything in packet
    memcpy(m, opC, sizeof(opC));
    memcpy(m + 1, opA, sizeof(opA));
    memcpy(m + 5, opB, sizeof(opB));
    memcpy(m + 9, answer, sizeof(answer));
}

void unmake_packet(char *recvM) {
    int i;
    uint32_t a = 0;
    uint32_t b = 0;
    char c;
    // Covert b from bytes
    for (i = 9; i >= 6; i--) {
        b += recvM[i];
        if (i > 9) {
            b = b << 8;}
    }
    // Covert a from bytes
    for (i = 5; i >= 2; i--) {
        a += recvM[i];
        if (i > 6) {
            a = a << 8; }
    }
    // Covert c from bytes
    for (i = 1; i >= 0; i--) {
        c += recvM[i];
        if (i > 1) {
            c = c << 1; }
    }
}

uint32_t calc(uint32_t a, uint32_t b, char c) {
    uint32_t result = 0;
    switch(c){
		case '+':
			result = a + b;
			break;
		case '-':
		        result = a-b;
			break;
		case '/':
		        result = a/b;	
			break;
		case '*':
			result = a*b;
			break;
	}
    return result;
}