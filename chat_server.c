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


int main(int argc, char *argv[]){
	int serverSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);
	chatSession(serverSocket, portNumber);
	return 0;
}

int chatSession(int serverSocket, int portNumber) {
	fd_set rfds;
	struct clientNode *curNode= NULL;
	struct clientNode *headClientNode = curNode;
	int maxSocket = serverSocket; //need this because we are going to loop throug hte set of sockets used and we need to know where to stop looking 
	int ret;

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
		}
		//now check for client activity by looping through the set of sockets
		curNode = headClientNode;
		while(curNode != NULL) {
			//check if curNode's socket is set
			if (FD_ISSET(curNode->socket, &rfds)) {
				ret = clientActivity(curNode->socket, &headClientNode);
				if (ret > 0) {
					curNode = NULL;
				}
			}
			if (curNode != NULL) {
				curNode = curNode->next;
			}
		}
	}
} 

int clientActivity(int clientSocket, struct clientNode **head) {
	char buf[MAX_PACKET_SIZE];
	uint8_t byteFlag;
	struct chat_header cheader;
	if (recievePacket(clientSocket, buf) == 1){
		removeClientNode(head, clientSocket);
		close(clientSocket);
		return 1;
	}

	memcpy(&cheader, buf, sizeof(struct chat_header)); //gets the header from the recieved packet
	byteFlag = cheader.byteFlag; 
	if (byteFlag == 5 ) { //message flag
		messageRecieved(buf, cheader, *head, clientSocket);
	}
	else if (byteFlag == 8) { //client exiting flag
		clientExit(head, clientSocket);
		return clientSocket;
	}
	else if (byteFlag == 10) { //list handle flag
		
		listHandles(*head, clientSocket);
	}
	else {
		perror("Incomming byte flag is invalid");
		exit(-1);
	}
	return 0; 
}

int clientExit(struct clientNode **head ,int clientSocket) {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader;
	removeClientNode(head,clientSocket);
	cheader.byteFlag = 9;
	cheader.packetLen = htons(sizeof(struct chat_header));
	memcpy(packet, &cheader, sizeof(struct chat_header));
	sendPacket(clientSocket, packet, cheader);
	close(clientSocket);
	return 0;
}

int removeClientNode(struct clientNode **head, int socket) {
	struct clientNode *curNode;
	struct clientNode *temp;

	curNode = *head;
	if (socket == curNode->socket) {
		temp = curNode;
		*head = curNode->next;
		free(temp);
		return 0;
	}
	while(curNode->next != NULL) {
		if(socket == curNode->socket) {
			temp = curNode->next;
			curNode->next = curNode->next->next;
			free(temp);
			return 0;
		}
		temp = curNode;
		curNode = curNode->next;
	}
	if (socket == curNode->socket) {
		temp->next = NULL;
		free(curNode);
	}
	return 0;
}

int listHandles(struct clientNode *head, int socket) {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader;
	uint32_t handleCount = 0;
	struct clientNode *curNode = head;
	uint16_t packetSize;
	uint8_t handleLength;

	//first send flag = 11 packet
	cheader.byteFlag = 11;
	packetSize = sizeof(struct chat_header) + sizeof(uint32_t);
	cheader.packetLen = htons(packetSize);
	while(curNode != NULL) {
		handleCount++;
		curNode = curNode->next;
	}
	handleCount = htonl(handleCount);
	memcpy(packet, &cheader, sizeof(struct chat_header)); //header
	memcpy((packet + sizeof(struct chat_header)), &handleCount, sizeof(uint32_t));//num handles
	sendPacket(socket, packet, cheader);
	
	//now for the flag 12s
	curNode = head;
	cheader.byteFlag = 12; 

	while (curNode != NULL) { 
		handleLength = strlen(curNode->handle);
		packetSize = sizeof(struct chat_header) + handleLength + sizeof(uint8_t);
		cheader.packetLen = htons(packetSize);
		memcpy(packet, &cheader, sizeof(struct chat_header));
		memcpy(packet + sizeof(struct chat_header), &handleLength, sizeof(uint8_t)); 
		memcpy(packet + sizeof(struct chat_header)+sizeof(uint8_t), curNode->handle, handleLength);
		sendPacket(socket, packet, cheader);
		curNode = curNode->next; 
	}
	packetSize = sizeof(struct chat_header);
	cheader.byteFlag = 13;
	cheader.packetLen = htons(packetSize);
	memcpy(packet, &cheader, sizeof(struct chat_header));
	sendPacket(socket,packet,cheader);
	return 0;
}

int messageRecieved(char *recieved, struct chat_header cheader, struct clientNode *head, int sendingSocket) {
	char curHandle[MAX_HANDLE_LEN];
	uint8_t srcHandleLength, numDestinations, curHandleLen;
	int offset = sizeof(struct chat_header);
	int curSocket , i;

	//first we need to check if there are multiple destination handles:
	srcHandleLength = recieved[offset];
	offset += srcHandleLength + 1;
	numDestinations = recieved[offset++];

	for (i = 0; i < numDestinations; i++) {
		curHandleLen = *(recieved + offset++);
		memcpy(curHandle, recieved + offset, curHandleLen); //gets the dest name
		offset += curHandleLen;
		curHandle[curHandleLen] = '\0';
		curSocket = getSocket(curHandle, head);
		if (curSocket < 0) {
			sendInvalidDest(curSocket ,sendingSocket, curHandle, curHandleLen);
		}
		else {
			sendPacket(curSocket, recieved, cheader);
		}
	}
	return 0;
}

int sendInvalidDest(int destSocket ,int sendingSocket, char *destHandle, uint8_t destHandleLength) {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader;
	int offset = 0;

	cheader.byteFlag = 7;
	cheader.packetLen = htons(sizeof(struct chat_header) + destHandleLength + 1);

	memcpy(packet, &cheader, sizeof(struct chat_header));
	offset += sizeof(struct chat_header);
	memcpy(packet + offset, &destHandleLength, sizeof(uint8_t));
	offset++;
	memcpy(packet+offset, destHandle, destHandleLength);
	sendPacket(sendingSocket, packet, cheader);

	return 0;
}

int sendPacket(int socket, char *packet, struct chat_header cheader) {
	int sent;
	//send packet:
	sent =  send(socket, packet, ntohs(cheader.packetLen), 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
	if (sent ==0) {
		perror("sent zero");
		exit(-1);
	}
	return 0;
}

int getSocket(char *handle, struct clientNode *head) {
	struct clientNode *curNode = head;

	while (curNode != NULL) {
		if (strcmp(handle, curNode->handle) == 0) {
			//found the dest handle in our list
			return curNode->socket; 
		}
		curNode = curNode->next; 
	}

	return -1; //socket could not be found for this handle
}

int newClientConnection(int serverSocket,struct clientNode **head ){
	int clientSocket;
	char handle[MAX_HANDLE_LEN];
	uint8_t handleLength;
	char buf[MAX_PACKET_SIZE];

	if ((clientSocket = accept(serverSocket,(struct sockaddr*) 0, (socklen_t *) 0)) < 0) {
		perror("accept call error");
		exit(-1);
	}

	recievePacket(clientSocket, buf);
	memcpy(&handleLength, buf + sizeof(struct chat_header), sizeof(uint8_t));
	memcpy(handle, buf + sizeof(struct chat_header) + sizeof(uint8_t), handleLength); 
	handle[handleLength] = '\0';

	if (checkHandle(handle, *head) == 1) {
		//handle is invalid
		sendHandleExistsError(clientSocket);
	}
	else {
		//handle is valid
		sendValidHandle(clientSocket);
		addClient(head, handle, handleLength, clientSocket);
	}
	return 0;
}

int recievePacket(int socket, char *packet) {
	uint16_t packetLength;
	int messageLen;

	if ((messageLen = recv(socket, packet, sizeof(uint16_t), MSG_WAITALL)) < 2)
	{
		if (messageLen == 0) {
			return 1;
		}
	}
	memcpy(&packetLength, packet, sizeof(uint16_t));
	packetLength = ntohs(packetLength);

	messageLen += recv(socket, packet + sizeof(uint16_t), packetLength - sizeof(uint16_t),MSG_WAITALL);
	if (messageLen < packetLength) {
		perror("error recieveing packet");
	}
	return 0;
}

int addClient(struct clientNode **head, char *handle, int handleLen, int clientSocket) {
	struct clientNode *newClient = (struct clientNode *) calloc(1, sizeof(struct clientNode)); 
	struct clientNode *curNode; 
	
	newClient->socket = clientSocket;
	handle[handleLen] = '\0';
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

/*
int freeClientList(struct clientNode *head){
	struct clientNode *curNode = head;

	while (head != NULL) {
		curNode = head;
		head = head->next;
		free(curNode);
	}
	return 0;
}
*/

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

