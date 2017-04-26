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
	struct clientNode *curNode= NULL;
	struct clientNode *headClientNode = curNode;
	struct clientNode node;
	int clientSocket;
	int maxSocket = serverSocket;


//////////////////////////////////////////
	int count = 0;
///////////////////////////////////////////


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
			printf("\nA new client is about to attempt connection\n");
			curNode->next = newClientConnection(serverSocket, headClientNode);
			//printf("\nNode name is %s\n", node.handle);
			//memcpy(curNode->handle, node.handle, strlen(node.handle));
			//curNode->socket = node.socket

			printf("\nBefore if\n");
			if (curNode->next != NULL) {
				printf("\njust entered\n");
				curNode = curNode->next;
				printf("\nafter advancing\n");
			}
			
///////////////////////////////////////////////////////////
			printf("\nPRINT CLIENT LIST\n");
			curNode = headClientNode;
			while(curNode != NULL) {
				printf("count = %d  handle = %s\n", count++, curNode->handle);
				
				curNode = curNode->next; 
			}

/////////////////////////////////////////////////////////
			///////


			//checkHandle(curNode, head); //dont forget to free the node that was created if the handle already exists
						
		}
		/*
		//check for incoming client activity
		nodePtr = headClientNode;
		while(nodePtr != NULL) {
			if (FD_ISSET(nodePtr->socket, &rfds)) {
				clientActivity(nodePtr->socket);
         	}
			nodePtr = nodePtr->next;
		}
		*/
	}
	freeClientList(headClientNode);
} 

int clientActivity(int clientSocket) {
	char buf[MAXBUF];
	int recieved;
	struct chat_header cheader;
	printf("I GET TO CLIENT ACTIVITY AT LEAST \n");
	if ((recieved = recv(clientSocket, buf, MAXBUF, 0)) < 0) {
    	perror("Error recieveing incoming client packet \n");
    	exit(-1);
  	}
	if (recieved == 0) {
	   perror("Error: Read incoming client packet as zero bytes \n");
	   exit(-1);
   	}
	printf("I GET TO CLIENT ACTIVITY AT LEAST \n");

	return 0; 
}

struct clientNode * newClientConnection(int serverSocket,struct clientNode *head, struct clientNode *toAdd) {
	int clientSocket, messageLen;
	int handle[MAX_HANDLE_LEN];
	uint8_t handleLength;
	struct clientNode *nodePtr;
	char buf[MAXBUF];
	struct chat_header cheader;

	
	if ((clientSocket = accept(serverSocket,(struct sockaddr*) 0, (socklen_t *) 0)) < 0) {
		perror("accept call error");
		exit(-1);
	}
	printf("\n accept() call done\n");
	nodePtr = malloc(sizeof(struct clientNode));	
	nodePtr->next = NULL;	
	nodePtr->socket =  clientSocket;
	printf("\n node allocated");
	//recieve the clients initial packet containing handle and handle length
	if ((messageLen = recv(clientSocket, buf, MAXBUF, 0)) < 0)
	{
		perror("Initial Client Recieve Call Error");
		exit(-1);
	}
	if (messageLen == 0) {
		perror("Zero bytes received for initial packet");
		exit(-1);
	}
	printf("\n message recieved");
	memcpy(&cheader, buf, sizeof(struct chat_header));
	printf("\n cpy1, header flag = %d", cheader.byteFlag);
	memcpy(&handleLength, buf + sizeof(struct chat_header), sizeof(uint8_t));
	printf("\n cpy2 handle len: %d", handleLength);
	memcpy(nodePtr->handle, buf + sizeof(struct chat_header) + sizeof(uint8_t), handleLength); 
	printf("\n cpy3");


	printf("\nnodePtr has been created with handle %s and socket %d", nodePtr->handle, clientSocket);

	
	if (checkHandle(nodePtr, head) == 1) {
		//handdle is valid
		printf("Handle is INVALID (exists), sending error packet\n");
		sendHandleExistsError(clientSocket);
	}
	else {
		//handle is invalid
		printf("Handle is VALID, sending packet\n");
		sendValidHandle(clientSocket);
	}

	printf("Node to add has handle %s\n", nodePtr->handle);
	return nodePtr; 
}

int sendHandleExistsError(int serverSocket){
	//send flag =3;
	int sent;
	struct chat_header cheader;
	char *packet = malloc(sizeof(struct chat_header));

	cheader.packetLen = htons(sizeof(struct chat_header));
	cheader.byteFlag = 3;
	memcpy(packet, &cheader, ntohs(cheader.packetLen));
	sent =  send(serverSocket, packet, ntohs(cheader.packetLen), 0);
	if (sent < 0) {
		perror("flag = 3 send call error");
		exit(-1);
	}
	if (sent == 0) {
		perror("Error: sent zero bytes in response to initial packet");
		exit(-1);
	}

	free(packet);
	return 0;
}

int sendValidHandle(int serverSocket){
	//send flag =2;
	int sent;
	struct chat_header cheader;
	char *packet = malloc(sizeof(struct chat_header));

	cheader.packetLen = htons(sizeof(struct chat_header));
	cheader.byteFlag = 2;
	memcpy(packet, &cheader, ntohs(cheader.packetLen));
	sent =  send(serverSocket, packet, ntohs(cheader.packetLen), 0);
	if (sent < 0) {
		perror("flag = 2 send call error");
		exit(-1);
	}
	if (sent == 0) {
		perror("Error: sent zero bytes in response to initial packet");
		exit(-1);
	}
	free(packet);
	printf("\nVALID HANDLE HAS BEEN SENT\n");
	return 0;
}


int checkHandle(struct clientNode *nodePtr, struct clientNode *head) {
	//check if handle already exists
	struct clientNode *curNode = head;

	while(curNode != NULL) {
		printf("\nCurNode handle is: %s\n", curNode->handle);
		printf("\nnodePtr handle is: %s\n", nodePtr->handle);
		if (strcmp(curNode->handle, nodePtr->handle) == 0 && curNode != nodePtr){

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

