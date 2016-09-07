#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

# define MAXDATASIZE 5000
# define PATHLENGTH 1000
# define HOSTNAMELENGTH 1000

using namespace std;

int copy_buf(ofstream &file, char * buf, int numbytes) {
	char *ptr = buf;
	if (!file.is_open()){
		try {
			file.open("output", ios:: binary | ios::out | ios::trunc);
			if (strncmp(ptr, "HTTP/1.0 404 Not Found", 22) == 0 || strncmp(ptr, "HTTP/1.1 404 Not Found", 22) == 0) {
				file.write("<html><h1>404 NOT FOUND!</h1></html>", strlen("<html><h1>404 NOT FOUND!</h1></html>"));
				return 1;
			}
			if (strncmp(ptr, "HTTP/1.0 400 Bad Request", 24) == 0) {
				file.write("<html><h1>400 BAD REQUEST!</h1></html>", strlen("<html><h1>400 BAD REQUEST!</h1></html>"));
				return 2;	
			}
			if (strncmp(ptr, "HTTP/1.0 200 OK", 15) != 0 && strncmp(ptr, "HTTP/1.1 200 OK", 15) != 0) {
				printf("Server says: %s\n", ptr);
				return 3;
			}
			ptr = strstr(ptr, "\r\n\r\n");
			ptr += 4;
		}
		catch (ifstream::failure e){
			cout << "Exception opening file";
		}
	}
	file.write(ptr, numbytes - (ptr - buf));
	return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char*argv[]) {
	int sockfd, numbytes, rv;
	size_t i = 7, j = 0;
	char buf[MAXDATASIZE], get_message[1000], addr[PATHLENGTH+HOSTNAMELENGTH];
	char file_path[PATHLENGTH] = "index.html", hostname[HOSTNAMELENGTH], PORT[5] = "80";
	struct addrinfo hints, *servinfo, *p;
		if (argc != 2) {
		fprintf(stderr, "Usage: ./http_client <hostname>\n");
		exit(1);
	}
	memset(addr, '\0', sizeof addr);
	strcpy(addr, argv[1]);
	
	//extracting hostname
	if (addr[i] == '/')
		i++;
	while(addr[i] != '/' && addr[i] != ':' && addr[i] != '\0') {
		hostname[j] = addr[i];
		j++;
		i++;
	}
	hostname[j] = '\0';
	j = 0;
	//extracting PORT
	if (addr[i] == ':') {
		i++;	
		while(addr[i] != '/' && addr[i] != '\0') {
			PORT[j] = addr[i];
			j++;
			i++;
		}
		PORT[j] = '\0';
	}
	j = 0;
	//extracting path to file
	if (i <= strlen(addr)-2) {
		i++;
		while(addr[i] != '\0') {
			file_path[j] = addr[i];
			j++;
			i++;
		}
		file_path[j] = '\0';	
	}
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//getting address info of the server
	if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// creating socket and connecting with the server
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "clinet: failed to connect\n");
		return 2;
	}
	freeaddrinfo(servinfo); //don't need anymore
	// set SO_REUSEADDR on a socket to true (1):
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	
	//creating and sending GET message
	sprintf(get_message, "GET /%s HTTP/1.0\r\nUser-Agent: bhatti2 (sp16-cs438 mp1)\r\nAccept: */*\r\n", file_path);
	sprintf(get_message+ strlen(get_message), "Host: %s:%s\r\n", hostname, PORT);
	sprintf(get_message + strlen(get_message), "Connection: Keep-Alive\r\n\r\n");
	if(send(sockfd, get_message, strlen(get_message), 0) == -1) {
		perror("send");
		exit(3);
	}

	ofstream out_file;
	int ret_code = 0;
	while(1){
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
			perror ("recv");
			exit(4);
		}
		if (numbytes == 0)
			break;
		ret_code = copy_buf(out_file, buf, numbytes);
		if (ret_code != 0)
			break;
	}
	out_file.close();
	close(sockfd);
	return 0;
}
