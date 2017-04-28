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
	
	/* close the sockets */
	close(clientSocket);
	close(serverSocket);

	
	return 0;
}

int chatSession(int serverSocket, int portNumber) {
	fd_set rfds;
	struct clientNode *curNode= NULL;
	struct clientNode *headClientNode = curNode;
	int clientSocket;
	int maxSocket = serverSocket; //need this because we are going to loop throug hte set of sockets used and we need to know where to stop looking 

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
		if (select(maxSocket + 1 , &rfds,NULL,NULL,NULL) < 0  ){
			perror("server select call error");
			exit(-1);
		}

		//new connection to server socket 
		if (FD_ISSET(serverSocket, &rfds)) {
			newClientConnection(serverSocket, &headClientNode);
			/*printf("\nPRINT CLIENT LIST\n");
			curNode = headClientNode;
			while(curNode != NULL) {
				printf("element: %s\n", curNode->handle);				
				curNode = curNode->next; 
			}*/
		}
		//now check for client activity by looping through the set of sockets
		curNode = headClientNode;
		while(curNode != NULL) {
			//check if curNode's socket is set
			if (FD_ISSET(curNode->socket, &rfds)) {
				clientActivity(curNode->socket);
			}
			curNode = curNode->next;
		}
	}
	freeClientList(headClientNode);
} 

int clientActivity(int clientSocket) {
	char buf[MAXBUF];
	int recieved, packetLength;
	uint8_t byteFlag;
	struct chat_header cheader;

	if ((recieved = recv(clientSocket, buf, MAXBUF, 0)) < 0) {
    	perror("Error recieveing incoming client packet \n");
    	exit(-1);
  	}
	if (recieved == 0) {
	   perror("Error: Read incoming client packet as zero bytes \n");
	   exit(-1);
   	}
	printf("I GET TO CLIENT ACTIVITY AT LEAST \n");
	memcpy(&cheader, buf, sizeof(struct chat_header)); //gets the header from the recieved packet
	byteFlag = cheader.byteFlag; 
	packetLength = ntohs(cheader.packetLen);

	printf("Packet length recieved from client activity is %d and byte flag is: %d\n", packetLength, byteFlag);

	if (byteFlag == 5 ) { //message flag
		messageRecieved(buf, cheader);
	}
	else if (byteFlag == 8) { //client exiting flag

	}
	else if (byteFlag == 10) { //list handle flag

	}
	else {
		perror("Incomming byte flag is invalid");
		exit(-1);
	}
	return 0; 
}

int messageRecieved(char *recieved, struct chat_header cheader) {
	//char packet[MAX_PACKET_SIZE];
	uint8_t srcHandleLength, numDestinations;
	int offset = sizeof(struct chat_header);

	//first we need to check if there are multiple destination handles:
	//memcpy(&srcHandleLength, recieved + offset, sizeof(uint8_t)); //gets the src handle length so that I can get num destinations
	srcHandleLength = recieved[offset];
	offset += srcHandleLength + 1;
	numDestinations = recieved[offset];


	
	return 0;
}

int newClientConnection(int serverSocket,struct clientNode **head ){
	int clientSocket, messageLen;
	char handle[MAX_HANDLE_LEN];
	uint8_t handleLength;
	char buf[MAXBUF];
	struct chat_header cheader;

	if ((clientSocket = accept(serverSocket,(struct sockaddr*) 0, (socklen_t *) 0)) < 0) {
		perror("accept call error");
		exit(-1);
	}
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

	memcpy(&handleLength, buf + sizeof(struct chat_header), sizeof(uint8_t));
	memcpy(handle, buf + sizeof(struct chat_header) + sizeof(uint8_t), handleLength); 
	handle[handleLength] = '\0';

	if (checkHandle(handle, *head) == 1) {
		//handdle is invalid
		printf("Handle is INVALID (exists), sending error packet\n");
		sendHandleExistsError(clientSocket);
	}
	else {
		//handle is valid
		printf("Handle is VALID, sending packet\n");
		sendValidHandle(clientSocket);
		addClient(head, handle, handleLength, clientSocket);
	}
	return 0;
}

int addClient(struct clientNode **head, char *handle, int handleLen, int clientSocket) {
	struct clientNode *newClient = (struct clientNode *) malloc(sizeof(struct clientNode)); 
	struct clientNode *curNode; 
	
	newClient->socket = clientSocket;
	memcpy(newClient->handle, handle, handleLen);
	newClient->next = NULL;

	if (*head == NULL) {
		*head = newClient; 
	}
	else {
		//iterate to the end of the list
		curNode = *head;
		while (curNode->next != NULL) {
				curNode = curNode->next;
		}
		curNode->next = newClient;
	}
	return 0;
}

int sendHandleExistsError(int serverSocket){
	//send flag =3;
	int sent;
	struct chat_header cheader;
	char packet[MAX_PACKET_SIZE];

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

	return 0;
}

int sendValidHandle(int serverSocket){
	//send flag =2;
	int sent;
	struct chat_header cheader;
	char packet[MAX_PACKET_SIZE];

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
	return 0;
}

int checkHandle(char *handle, struct clientNode *head) {
	//check if handle already exists
	struct clientNode *curNode = head;

	while(curNode != NULL) {
		if (strcmp(curNode->handle, handle) == 0){
			return 1;
		}
		curNode = curNode->next;
	}
	return 0;
}

int freeClientList(struct clientNode *head){
	struct clientNode *curNode = head;

	while (head != NULL) {
		curNode = head;
		head = head->next;
		free(curNode);
	}
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

