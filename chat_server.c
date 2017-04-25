/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "chat_server.h"


int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

	chatSession(serverSocket, portNumber);
	// wait for client to connect
	//clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
	//recvFromClient(clientSocket);
	
	/* close the sockets */
	close(clientSocket);
	close(serverSocket);

	
	return 0;
}

int chatSession(int serverSocket, int portNumber) {
	fd_set rfds;
	struct clientNode *headClientNode = NULL;
	struct clientNode *curNode;
	int clientSocket;
	int maxSocket = serverSocket;


	while (1) {
		FD_ZERO(&rfds);
		FD_SET(serverSocket, &rfds ); //watch socket for update
		
		//go through list of connected clients and watch each of the sockets
		curNode = headClientNode; 
		while(curNode != NULL) {
			FD_SET(curNode->socket, &rfds);
			if (maxSocket < curNode->socket){
				maxSocket = curNode->socket;
			}
			curNode = curNode->next;
		}


		if (select(maxSocket + 1, &rfds,NULL,NULL,NULL) < 0  ){
			perror("server select call error");
			exit(-1);
		}

		//new connection to server socket 
		if (FD_ISSET(serverSocket, &rfds)) {
			curNode = newClientConnection(serverSocket, headClientNode);
			//checkHandle(curNode, head); //dont forget to free the node that was created if the handle already exists
						
		}
		//elseif...
	}
	freeClientList(headClientNode);
} 

struct clientNode * newClientConnection(int serverSocket,struct clientNode *head) {
	int clientSocket, messageLen;
	int handleLength, handle[MAX_HANDLE_LEN];
	struct clientNode *newNode = NULL;
	char buf[MAXBUF];
	struct chat_header cheader;


	if ((clientSocket = accept(serverSocket,(struct sockaddr*) 0, (socklen_t *) 0)) < 0) {
		perror("accept call error");
		exit(-1);
	}
	newNode = malloc(sizeof(struct clientNode));	
	newNode->next = NULL;	
	newNode->socket =  clientSocket;

	//recieve the clients initial packet containing handle and handle length
	if ((messageLen = recv(clientSocket, buf, MAXBUF, 0)) < 0)
	{
		perror("Initial Client Recieve Call Error");
		exit(-1);
	}

	memcpy(&cheader, buf, sizeof(struct chat_header));
	memcpy(&handleLength, buf + sizeof(struct chat_header), sizeof(uint8_t));
	memcpy(newNode->handle, buf + sizeof(struct chat_header) + sizeof(uint8_t), handleLength); 


	printf("\nNewNode has been created with handle %s and socket %d\n", newNode->handle, clientSocket);

	
	if (checkHandle(newNode, head) == 1) {
		//handdle is valid
		printf("Handle is INVALID (exists), sending error packet\n");
		sendHandleExistsError(serverSocket);
	}
	else {
		//handle is invalid
		printf("Handle is VALID, sending packet\n");
		sendValidHandle(serverSocket);
	}

	return newNode; 
}

int sendHandleExistsError(int serverSocket){
	//send flag =3;
	int sent;
	struct chat_header cheader;
	//char *packet = malloc(sizeof(struct chat_header));

	cheader.packetLen = htons(sizeof(struct chat_header));
	cheader.byteFlag = 3;

	sent =  send(serverSocket, &cheader, ntohs(cheader.packetLen), 0);
	if (sent < 0) {
		perror("flag = 3 send call error");
		exit(-1);
	}

	//free(packet);
	return 0;
}

int sendValidHandle(int serverSocket){
	//send flag =2;
	int sent;
	struct chat_header cheader;
	//char *packet = malloc(sizeof(struct chat_header));

	cheader.packetLen = htons(sizeof(struct chat_header));
	cheader.byteFlag = 2;

	sent =  send(serverSocket, &cheader, ntohs(cheader.packetLen), 0);
	if (sent < 0) {
		perror("flag = 2 send call error");
		exit(-1);
	}


	//free(packet);
	return 0;
}


int checkHandle(struct clientNode *newNode, struct clientNode *head) {
	//check if handle already exists
	struct clientNode *curNode = head;

	while(curNode != NULL) {
		if (strcmp(curNode->handle, newNode->handle) == 0){//if the handles are equal send packet with flag ==3
			return 1;
		}
		curNode = curNode->next;
	}
	return 0;
}



int freeClientList(struct clientNode *head){
	return 0;
}


int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

