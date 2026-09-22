#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

#define FD_TAB_SIZE 128

/*void echo_server(int sockfd) {
	char buff[MSG_LEN];
	while (1) {
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
		printf("Received: %s", buff);
		// Sending message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
	}
}*/

static int handle_client(int fd) {
	char buff[MSG_LEN];
	memset(buff, 0, MSG_LEN);

	int ret = recv(fd, buff, MSG_LEN, 0);
	if (ret <= 0) {
		return 0;   // client déconnecté
	}

	printf("Received (fd %d): %s", fd, buff);

	if (send(fd, buff, strlen(buff), 0) <= 0) {
		perror("send()");
		return 0;
	}
	printf("Message sent!\n");
	return 1;       // tout va bien, on continue
}

int handle_bind(char* serv_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, serv_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int main(int argc, char* argv[]) {
	//struct sockaddr cli;
	//int sfd, connfd;
	char* port = argv[1];


	//socklen_t len;
	int sfd = handle_bind(port);
	if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		close(sfd);
		exit(EXIT_FAILURE);
	}
	//len = sizeof(cli);

	struct pollfd fds[FD_TAB_SIZE];
    //first item --> listening fd, event = POLLIN, revent = 0
    fds[0].fd = sfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    for (int i = 1; i < FD_TAB_SIZE; i++){
        fds[i].fd = -1;
        fds[i].events = 0 ;
        fds[i].revents = 0;
    }
    //all others --> -1, event = 0, revent = 0 

    while(1){
        printf("Will poll ...\n");
        int nbfds = poll(fds, FD_TAB_SIZE, -1);
        printf("Number of active fd (%d)\n", nbfds);
        for(int i = 0; i<FD_TAB_SIZE; i++){
            //if activity on listening socket
            //printf("Is running");
            if(i == 0 && (fds[0].revents & POLLIN)){
                //can accept
                fds[i].revents  = 0;
                int new_fd = accept(sfd, NULL, NULL);
				if (new_fd < 0){
					perror("accept()");
				} else {
					for (int j = 0; j<FD_TAB_SIZE; j++){  //peut etre qu'il faut commencer par 1
                    	if(fds[j].fd == -1){
                        	fds[j].fd = new_fd;
                        	fds[j].events = POLLIN;
                        	fds[j].revents = 0;
							printf("New client connected (fd %d)\n", new_fd);
                        	break;
                    	}
					}
                }
                //use new fd to init struct and fill array
                
            }

		}

			for (int i = 1; i < FD_TAB_SIZE; i++) {
				if (fds[i].fd == -1 || !(fds[i].revents & POLLIN)) {
					continue;
				}
				fds[i].revents = 0;
 
				if (!handle_client(fds[i].fd)) {
					printf("Connection ended (fd %d)\n", fds[i].fd);
					close(fds[i].fd);
					fds[i].fd = -1;
					fds[i].events = 0;
				}
			}
		}

	close(sfd);
	return EXIT_SUCCESS;
}

