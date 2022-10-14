/*
 * Group: Mary Mitchell and Maram Aldiabat
 * COMP 5320/6320
 * Lab 1.1: server11.c
 * Based on beej listener.c
 * 
 * compile: gcc server11.c -o server11
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#define MYPORT "10010"	// the port users will be connecting to

#define MAXBUFLEN 1024
#define MAXLINE 1024 // Max string size
#define MAXSIZE 1038 // Max packet size
#define LISTENQ 8

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
 else
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_in their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET_ADDRSTRLEN];
    // Packet variables
	//uint16_t msgLength; // 2 bytes
	//uint32_t seqNum = 0; // 4 bytes
	//const unsigned int MAXSEQNUM = (long unsigned int) pow(2, 32) - 1; // Maximum sequence number = 2^32 - 1
	//unsigned long timeStamp; // 8 bytes
	//struct timeval sendTime, receiveTime;
	//unsigned long roundTripTime;
	//char str[MAXLINE], sendBuf[MAXSIZE], recvBuf[MAXSIZE]; // Message and packet buffers
	char exitStr[MAXLINE] = "exit()\n";
    int pid; 

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 0;
	}
    /// have to do multi process
    // loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break; // if it binds then break and go to send and recinfo
	}

	if (p == NULL) { // if it doesnot bind with any request then exit all the server prog 
		fprintf(stderr, "listener: failed to bind socket\n");
		return 0;
	}

	freeaddrinfo(servinfo);

    bool flag = true;
    while(flag){
        // Receive the packet from the client
        printf("server11: waiting to recvfrom...\n");
        addr_len = sizeof their_addr;
	    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		    (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		    perror("recvfrom");
		    return 0;
	    }

        if (numbytes > 0){
            buf[numbytes] = '\0';
            printf("listener: packet contains \"%s\"\n", buf);
            if (strcmp(buf, exitStr) == 0) {
			    flag = false;
		    }
            pid = fork();

            if(pid == 0){
                if ((numbytes = sendto(sockfd, buf, numbytes, 0,
                             (const struct sockaddr *)&their_addr, sizeof their_addr)) == -1){
                    perror("sendto");
                    return 0;
                }    
                
            }
            if (pid < 0){
                perror("bad Fork Job");
                return 0;
            }

        }
//         printf("listener: got packet from %s\n",
// 		    inet_ntop(their_addr.ss_family,
// 			    get_in_addr((struct sockaddr *)&their_addr),
// 			    s, sizeof s));

    }

	close(sockfd);

	return 0;
}
