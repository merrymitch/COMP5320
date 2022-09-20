/*
 * Group: Mary Mitchell and Maram Aldiabat
 * COMP 5320/6320
 * Lab 1.1: client11c.c (Author: Mary Mitchell)
 * Based on beej talker.c
 * 
 * compile: gcc client11c.c -o client11c
 * run: ./client11c <server name>
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
#define MAXSIZE 1038 // Max packet size
#define MAXLINE 1024 // Max string size
#define NUMPACKETS 10000 // Number of packets the parent will send

/********** Method Declarations **********/
void make_packet(uint16_t msglength, uint32_t seqnum, unsigned long timestamp, char *numberstr, char *sendbuf);
void unmake_packet(int numbytes, char *recvbuf, unsigned long *stats);

int main(int argc, char **argv) {
    /********** Socket Variables **********/
    int sockfd, rv; 
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage servaddr;
    socklen_t servAddrLen;
    struct timeval timeout;
    timeout.tv_sec = 6; 
    timeout.tv_usec = 0;

    // Check for the command line arguments
	if (argc != 2) {
		fprintf(stderr,"usage: talker hostname\n");
		exit(1);
	}

    /********** Prepare Socket **********/
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERV_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(2);
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
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

    // Added timeout to alert child process when it is done receiving
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != 0) {
        perror("talker: failed to set opt");
        exit(4);
    }

    /********** Two Processes **********/
    int pid;
    pid = fork();

    if (pid == 0) { // Child process: receiver
        // Child process variables
        servAddrLen = sizeof servaddr;
        int received[NUMPACKETS + 1];
        int numbytes;
        int numLost = 0;
        char recvBuf[MAXSIZE];
        unsigned long recvTimeStamp;
        unsigned long statInfo[2]; // Only need sequence number and timestamp
        unsigned long smallestRTT = LONG_MAX;
        unsigned long largestRTT = 0;
        unsigned long averageRTT = 0;
        
        // Set everything in the received array as 0
        memset(&received, 0, sizeof(received));

        // Start receiving all the packets
        printf("Child process starting to receive\n");
        printf("BE PATIENT! MAY TAKE SEVERAL SECONDS FOR ALL PROCESSES TO COMPLETE!\n");
        while (1) {
            // Receive a packet
            if ((numbytes = recvfrom(sockfd, recvBuf, MAXSIZE, 0, (struct sockaddr *) &servaddr, &servAddrLen)) == -1) {
                perror("\nChild process is done receiving");
                break;
            }

            // Get the necessary info from the packet
            unmake_packet(numbytes, recvBuf, statInfo);

            // Mark it as being received
            received[(int)statInfo[0]] = 1;

            // Get the time the packet was received
            struct timeval recvTime;
            gettimeofday(&recvTime, NULL);
            recvTimeStamp = recvTime.tv_sec * 1000 + recvTime.tv_usec / 1000;

            // Calculate the round trip time of the packet
            unsigned long roundTripTime = recvTimeStamp - statInfo[1];

            // Add the rtt to the average 
            averageRTT += roundTripTime;

            // Determine if it had the smallest or biggest round trip time
            if (roundTripTime < smallestRTT) {
                smallestRTT = roundTripTime;
            }
            else if (roundTripTime > largestRTT) {
                largestRTT = roundTripTime;
            }

        }

        // Based on the values set in received, determine the amount of packets lost
        int i;
        for(i = 1; i < NUMPACKETS + 1; i++) {
            if (received[i] == 0) {
                numLost++;
            }
        }

        // Calculate the average rtt of the packets received
        averageRTT = averageRTT / (NUMPACKETS - numLost);

        // Print out the stats
        printf("=================================\n");
        printf("Statistical Information\n");
        printf("\tAmount of Missing Echo Packets: %d\n", numLost);
        printf("\tSmallest Round Trip Time: %lu ms\n", smallestRTT);
        printf("\tLargest Round Trip Time: %lu ms\n", largestRTT);
        printf("\tAverage Round Trip Time: %lu ms\n", averageRTT);
        printf("=================================\n");

    } 
    else { // Parent process: sender
        // Parent process variables
        int i, numbytes, number;
        uint16_t msgLength;
        uint32_t seqNum;
        unsigned long timeStamp;
        char numberStr[MAXLINE], sendBuf[MAXSIZE];
        
        // Send out the 10000 packets
        printf("Parent process starting to send\n");
        for (i = 1; i < NUMPACKETS + 1; i++) {
            // Get the number being sent
            number = i;
            sprintf(numberStr, "%d", number);

            // Get the timestamp 
            struct timeval sendTime;
            gettimeofday(&sendTime, NULL);
            timeStamp = sendTime.tv_sec * 1000 + sendTime.tv_usec / 1000;

            // Get the sequence number of the packet
            seqNum = i;

            // Get the total length of the message
            msgLength = 14 + strlen(numberStr);

            // Put everything into the packet
            make_packet(msgLength, seqNum, timeStamp, numberStr, sendBuf);

            // Send the packet
            if ((numbytes = sendto(sockfd, sendBuf, msgLength, 0, 
                servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
                    perror("problem sending packet");
			    	exit(5);
            }
        }
        printf("Parent process finished sending\n");
    }

    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;

}

/********** Method Definitions **********/
// Puts all the necessary info into a packet
void make_packet(uint16_t msglength, uint32_t seqnum, unsigned long timestamp, char *numberstr, char *sendbuf) {
    unsigned char msgLenArray[2];
    unsigned char seqNumArray[4];
    unsigned char timeStampArray[8];

    // Covert the numbers to bytes
    int i;
    for (i = 0; i < 2; i++) {
        msgLenArray[i] = (msglength >> (i * 8)) & 0xff;
    }
    for(i = 0; i < 4; i++) {
        seqNumArray[i] = (seqnum >> (i * 8)) & 0xff;
    }
    for(i = 0; i < 8; i++) {
        timeStampArray[i] = (timestamp >> (i * 8)) & 0xff;
    }

    // Put everything in the packet
    memcpy(sendbuf, msgLenArray, sizeof(msgLenArray));
    memcpy(sendbuf + 2, seqNumArray, sizeof(seqNumArray));
    memcpy(sendbuf + 6, timeStampArray, sizeof(timeStampArray));
    memcpy(sendbuf + 14, numberstr, strlen(numberstr));
}

// Extracts necessary info from packet for the statistics calculations
void unmake_packet(int numbytes, char *recvbuf,  unsigned long *stats) {
    unsigned char seqNumArray[4];
    unsigned char timeStampArray[8];

    // Prepare arrays by setting them to 0
    memset(&seqNumArray, 0, sizeof(seqNumArray));
    memset(&timeStampArray, 0, sizeof(timeStampArray));

    // Get the data from the received buffer
    int i;
    for(i = 0; i < 4; i++) {
        seqNumArray[i] = recvbuf[i + 2];
    }
    for(i = 0; i < 8; i++) {
        timeStampArray[i] = recvbuf[i + 6];
    }

    // Convert seqNum and timeStamp from bytes since they are the only necessary info
    unsigned long byteSQ = 0;
    for (i = 3; i >= 0; i--) {
        byteSQ += seqNumArray[i];
        if (i != 0) {
            byteSQ = byteSQ << 8;
        }
    }
    // Put necessary info in the stat array for calculations
    stats[0] = byteSQ; 

    unsigned long byteTS = 0;
    for(i = 7; i >= 0; i--) {
        byteTS += timeStampArray[i];
        if (i != 0) {
            byteTS = byteTS << 8;
        }
    }
    // Put necessary info in the stat array for calculations
    stats[1] = byteTS;
}