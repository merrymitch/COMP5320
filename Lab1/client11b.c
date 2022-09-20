/*
 * Group: Mary Mitchell and Maram Aldiabat
 * COMP 5320/6320
 * Lab 1.1: client11b.c (Author: Mary Mitchell)
 * Based on beej talker.c
 * 
 * compile: gcc client11b.c -o client11b
 * run: ./client11b <server name>
 * note: run server11 first
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

#define SERV_PORT "10010"
#define MAXLINE 1024 // Max string size
#define MAXSIZE 1038 // Max packet size

/********** Method Declarations **********/
void make_packet(uint16_t msglength, uint32_t seqnum, unsigned long timestamp, char str[], char *sendbuf);

/********** Main Function **********/
int main(int argc, char **argv) {
	/********** Variable Declarations **********/
	// talker.c variables
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_in servaddr;
	socklen_t serverAddrLen;
	// Packet variables
	uint16_t msgLength; // 2 bytes
	uint32_t seqNum = 0; // 4 bytes
	const unsigned int MAXSEQNUM = (long unsigned int) pow(2, 32) - 1; // Maximum sequence number = 2^32 - 1
	unsigned long timeStamp; // 8 bytes
	struct timeval sendTime, receiveTime;
	unsigned long roundTripTime;
	char str[MAXLINE], sendBuf[MAXSIZE], recvBuf[MAXSIZE]; // Message and packet buffers
	char exitStr[MAXLINE] = "exit()\n";

	// Check for the command line arguments
	if (argc != 2) {
		fprintf(stderr,"usage: talker hostname\n");
		exit(1);
	}
	
	/********** talker.c code **********/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo(argv[1], SERV_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(2);
	}

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
                perror("talker: socket");
                continue;
        }

        break;
    }

	if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        exit(3);
    }	

	/********** Send and Receive with Server **********/
	while (true) {
		// Get the string from the user
		printf("Enter string to send to server (\"exit()\" to leave): ");
		fgets(str, MAXLINE, stdin);
		// Check if user wants to exit program
		if (strcmp(str, exitStr) == 0) {
			break;
		}
		str[strlen(str) - 1] = '\0';
			
		// Get the total message length 
		msgLength = 14 + strlen(str);
			
		// Get the sequence number
		if (seqNum > MAXSEQNUM) {
			seqNum = 0;
		} 
		seqNum++;

		// Get the timestamp
		gettimeofday(&sendTime, NULL);	
		timeStamp = sendTime.tv_sec * 1000 + sendTime.tv_usec / 1000; // covert to ms
			
		// Assemble the packet
		make_packet(msgLength, seqNum, timeStamp, str, sendBuf);

		// Send the packet to the server
		if ((numbytes = sendto(sockfd, sendBuf, msgLength, 0, 
			p->ai_addr, p->ai_addrlen)) == -1) {
				perror("problem sending packet");
				exit(4);
		}
		printf("\n*********************************\n");
		printf("Packet sent to server:\n");
		printf("\tMessage Length: %lu\n", (unsigned long) msgLength);
		printf("\tSequence Number: %lu\n", (unsigned long) seqNum);
		printf("\tTimestamp: %lu\n", timeStamp);
		printf("\tString: %s\n", str);
		printf("*********************************\n\n");
		serverAddrLen = sizeof(servaddr);

		// Receive the packet from the server
		if ((numbytes = recvfrom(sockfd, recvBuf, MAXSIZE, 0, (struct sockaddr *) &servaddr, &serverAddrLen)) == -1) {
			perror("problem receiving packet");
			exit(5);
		}
		printf("=================================\n");
		printf("Packet received from server:\n");
			
		// Get the round trip time of the packet
		gettimeofday(&receiveTime, NULL);
	    roundTripTime = (receiveTime.tv_sec * 1000 + receiveTime.tv_usec / 1000) - (sendTime.tv_sec * 1000 + sendTime.tv_usec / 1000);
		
		// Print the message and round trip time of the packet
		printf("\tMessage from server: %s\n", recvBuf + 14);
		printf("\tRound trip time: ~ %lu ms\n", roundTripTime);
		printf("=================================\n\n");

		// Reset everything
		memset(str, 0, MAXLINE);
		memset(sendBuf, 0, MAXSIZE);
		memset(recvBuf, 0, MAXSIZE);	 
	}

	// Close and exit
	freeaddrinfo(servinfo);
	close(sockfd);

	return 0;

}

/********** Method Definitions **********/
/*
 * Prepare the packet to be sent to the server.
 */
void make_packet(uint16_t msglength, uint32_t seqnum, unsigned long timestamp, char str[], char *sendbuf) {
	// Array length corresponds to number of bytes
	char msgLenArray[2];
	char seqNumArray[4];
	char timeStampArray[8];

	// Covert the numbers to bytes
	int i;
	for (i = 0; i < sizeof(msglength); i++) {
		msgLenArray[i] = (msglength >> (i * 8)) & 0xff;
	}

	int j;
	for (j = 0; j < sizeof(seqnum); j++) {
		seqNumArray[j] = (seqnum >> (j * 8)) & 0xff;
	}

	int k;
	for (k = 0; k < sizeof(timestamp); k++) {
		timeStampArray[k] = (timestamp >> (k * 8)) & 0xff;
	}

	// Put everything in the packet
	memcpy(sendbuf, msgLenArray, sizeof(msgLenArray));
	memcpy(sendbuf + 2, seqNumArray, sizeof(seqNumArray));
	memcpy(sendbuf + 6, timeStampArray, sizeof(timeStampArray));
	memcpy(sendbuf + 14, str, strlen(str)); 
}