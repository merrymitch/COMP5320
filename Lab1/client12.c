/*
 * Group: Mary Mitchell and Maram Aldiabat
 * COMP 5320/6320
 * Lab 1.2: client12.c (Author: Mary Mitchell)
 * Based on lecture code
 * 
 * compile: gcc client12.c -o client12
 * run: ./client12 <server name> a b c
 * note: run server12 first
 */

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

#define SERV_PORT 10020
#define SEND_LENGTH 9
#define RECV_LENGTH 14

/********** Method Declarations **********/
void make_packet(uint32_t a, uint32_t b, char c, char *m);
uint32_t unmake_packet(char *recvM);

/********** Main Function **********/
int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;
    struct hostent *he;
    uint32_t a;
    uint32_t b;
    char c;
    char M[9];
    char recvM[14];
    uint32_t answer = 0;

    // Check for the command line arguments
	if (argc != 5) {
		fprintf(stderr,"usage: talker hostname\n");
		exit(1);
	}

    if ((he = gethostbyname(argv[1])) == NULL) {
		perror("Problem in resolving server IP Address.");
		exit(2);
	}

    // Create a socket for the client
    // If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(3);
    }
    
    //Creation of the socket address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr = *((struct in_addr *)he->h_addr);
    servaddr.sin_port =  htons(SERV_PORT); 

    //Connection of the client to the socket 
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        perror("Problem in connecting to the server");
        exit(4);
    }

    // Get the command line arguments and assign them 
    a = (unsigned)atoi(argv[2]);
    b = (unsigned)atoi(argv[3]);
    c = *argv[4];

    // Check that 'c' is a valid operator
    if(!(c == '+' || c == '-' || c == 'x' || c == '/')) {
        printf("Invalid operator");
        exit(5);
    }

    // Put everything in a packet
    make_packet(a, b, c, M);

    // Send the packet
    send(sockfd, M, SEND_LENGTH, 0);

    // Receive the packet
    if (recv(sockfd, recvM, RECV_LENGTH, 0) == 0) { // waiting to receive from server
        // error: server terminated prematurely
        perror("The server terminated prematurely");
        exit(6);
    }

    // Get the packet and extract the necessary information
    answer = unmake_packet(recvM);

    // Print the answer
    printf("Answer: %d\n", answer);

    // Check if answer is invalid, if so inform user
    if (recvM[13] != '1') {
        printf("Answer is invalid\n");
    }

    // Close socket
    close(sockfd);
    return 0;
     
}

/********** Method Definitions **********/
// Put everything in packet
void make_packet(uint32_t a, uint32_t b, char c, char *m) {
    int i;
    char opC[1];
    //uint8_t hex;
    char opA[4];
    char opB[4];

    // Not sure if this part is necessary since c can just be put in packet as is
    // Get hex/decimal value of c
    //if (c == '+') {
        //hex = 43;
    //} else if (c == '-') {
        //hex = 45;
    //} else if (c == 'x') {
        //hex = 120;
    //} else {
        //hex = 47;
    //}
    
    // Covert numbers to bytes
    // i = 0;
    //opC[0] = (hex >> (i * 8)) & 0xFF;
    for (i = 0; i < 4; i++) {
        opA[i] = (a >> (i * 8)) & 0xFF;
        opB[i] = (b >> (i * 8)) & 0xFF;
    }
    
    // Just put c in the temp array to be placed in the packet
    opC[0] = c;

    // Put eveything in packet
    memcpy(m, opC, sizeof(opC));
    memcpy(m + 1, opA, sizeof(opA));
    memcpy(m + 5, opB, sizeof(opB));
}

// Get the answer from the received packet
uint32_t unmake_packet(char *recvM) {
    int i;
    uint32_t answer = 0;

    // Covert from bytes
    for (i = 12; i >= 9; i--) {
        answer += recvM[i];
        if (i > 9) {
            answer = answer << 8;
        }
    }
    
    return answer;
}
