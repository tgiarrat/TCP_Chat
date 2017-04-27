/******************************************************************************
* tcp_client.c
*
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
#include <ctype.h>
#include <stdint.h>
#include "networks.h"
#include "chat_client.h"

char handle[MAX_HANDLE_LEN];

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor, will be the socket of the server i believe

	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	strcpy(handle, argv[1]);

	//sendToServer(socketNum);
	chatSession(socketNum);
	
	close(socketNum);
	
	return 0;
}

void chatSession(int socketNum) {
	fd_set rfds;

	sendInitialPacket(socketNum);

	while (1) { //1 is possibly temporary, need to run client until the user exits the client
		FD_ZERO(&rfds);
		FD_SET(STD_IN, &rfds); //watch std in
		FD_SET(socketNum, &rfds ); //watch socket for update
		
		if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
        	perror("select call error\n");
        	exit(-1);
      	}	
		
		//server update, read from server
		if (FD_ISSET(socketNum, &rfds)) {

		}
		//keyboard update, read from keyboard
		else if (FD_ISSET(0, &rfds)) { //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//MAYBE NOT ELSE IF, MGHT JUST BE IF
			localInput(socketNum);
		}

	}

}

int localInput(int socketNum) {
	char textBuffer[MAXBUF];
	int textLength;
	char commandType;
	
	fgets(textBuffer, MAXBUF, stdin);
	textBuffer[strcspn(textBuffer, "\n")] = 0;
	if (textBuffer[0] != '%') {
		printf("Incorrect message formatting\n");
		return -1;
	}

	commandType = toupper(textBuffer[1]); //letter of the command will be here in a properly formatted message

	if (commandType == 'M') { //send message
		if (strlen(textBuffer) > MAX_MSG_LEN) {
			perror("message entered is greater than 1000 characters");
		}
		message(textBuffer + COMMAND_OFFSET, socketNum);
		

	}
	else if (commandType == 'B') { //block user
		//block(textBuffer);
	}
	else if (commandType == 'U') { //unblock user
		//unblock(textBuffer);
	}
	else if (commandType == 'L') { //list handles
		//listHandles(textBuffer);
	}
	else if (commandType == 'E') { //exit
		//exitServer(textBuffer);
	}
	else {
		printf("Error: %c is an invalid command type\n", commandType);
		return -1;
	}

	return 0;

}

int sendInitialPacket(int socketNum){
	//char *packet;
	char packet[MAX_PACKET_SIZE];
	char *packetPtr;
	int packetLength, handleLen, sent, recieved;
	struct chat_header cheader;
	char incomingBuffer[MAXBUF];
	uint8_t flag;

	packetLength = sizeof(struct chat_header) + 1 + strlen(handle);
	printf("\nInitial packet size is: %d\n", packetLength);
	//packet = malloc(packetLength);
	packetPtr = packet;

	cheader.packetLen = htons(packetLength);
	cheader.byteFlag = 1;
	memcpy(packetPtr, &cheader, sizeof(struct chat_header));
	packetPtr += sizeof(struct chat_header);
	handleLen = strlen(handle);
	memcpy(packetPtr, &handleLen, sizeof(uint8_t)); //copy handle length
	packetPtr+= sizeof(uint8_t);
	memcpy(packetPtr, handle, handleLen); //copy handle

	//send packet
	printf("Sending Initial packet...\n");
	sent =  send(socketNum, packet, packetLength, 0);
	if (sent < 0)
	{
		perror("flag = 1 send call");
		exit(-1);
	}
	if (sent == 0) {
		perror("sent 0 bytes instead of initial header packet");
		exit(-1);
	}
	printf("Recieveing packet response after sending intitial packet...\n");
	recieved = recv(socketNum, incomingBuffer, MAXBUF, 0);
	if (recieved < 0) {
		perror("Initial packet recieved no response from the server");
		exit(-1);
	}
	if (recieved == 0) {
		perror("Recieved zero bytes in response to initial packet");
		exit(-1);
	}

	memcpy(&flag, incomingBuffer + sizeof(uint16_t), sizeof(uint8_t)); //gets the flag from the incoming buffer

	if (flag == 3) {
		printf("Invalid Handle Flag = 3\n");
	}
	else if (flag ==2) {
		printf("Valid handle Flag = 2\n");
	}
	else {
		printf("uh oh... flag != 2 or 3\n");
		printf("flag is %d, recieved %d\n", flag, recieved);
	}
	return 0;
}

int message(char *textBuffer, int socketNum) {
	char packet[MAX_PACKET_SIZE]; //this will be the entire packet. I knoe its a lil confusing but oh well too late
	char *packetPtr;
	char *arg;
	char **destHandles;
	char *curDest;
	struct chat_header cheader;
	uint8_t srcLength, numDestinations ;
	int messageLength, i, destLen, sent;
	int destHandleTotal = 0;

	cheader.byteFlag = 5;
	srcLength = strlen(handle);
	
	arg = strtok(textBuffer, " "); //get a space separated token of the string 
	if (isdigit(arg[0])) {
		printf("FIRST ARG IS A DIGIT \n");
		numDestinations = atoi(arg);
	}
	else {
		printf("FIRST ARG IS NOT A DIGIT\n");
		numDestinations = 1;
	}

	destHandles = malloc(numDestinations);

	

	for (i = 0; (i < numDestinations) && (arg != NULL); i++) {
		arg = strtok (NULL, " "); //arg is a dest handle
		*(destHandles + i) = malloc(strlen(arg));
		memcpy( (*(destHandles+i)) , arg, strlen(arg));
		destHandleTotal += strlen(arg);
	}
	if (i != numDestinations){
		printf("Error: Incorrect number of destination handles entered\n");
		return -1;
	}
	arg = strtok (NULL, "\n"); //arg is message
	messageLength = strlen(arg) + 1;  // plus one is for the null terminating character at the end
	printf("text is %s", arg);
	
	cheader.packetLen =
		htons(sizeof(struct chat_header) + srcLength + messageLength 
		+ numDestinations + 2 + destHandleTotal);
	printf("\n");
	printf("-----------PRINTING THE PACKET INFO---------------------\n");
	printf("text is %s\n", arg);
	printf("Packet Length is: %d\n", ntohs(cheader.packetLen));
	printf("Handle is: %s\n", handle);
	printf("first dest is: %s\n", *destHandles);
	printf("second dest is: %s\n", *(destHandles + 1));	
	printf("\n");

	//begin making packet:
	packetPtr = packet;
	memcpy(packetPtr, &cheader, sizeof(struct chat_header)); //copy chat header
	packetPtr += sizeof(struct chat_header);
	memcpy(packetPtr, &srcLength, sizeof(uint8_t));
	packetPtr += sizeof(uint8_t);
	memcpy(packetPtr, handle, srcLength);
	packetPtr += srcLength;
	memcpy(packetPtr, &numDestinations, sizeof(uint8_t));
	packetPtr += sizeof(uint8_t);
	//copy each of the destinations
	for (i = 0; i < numDestinations; i++) {
		curDest = *(destHandles+i);
		destLen = strlen(curDest);
		memcpy(packetPtr, &destLen, sizeof(uint8_t));
		packetPtr += sizeof(uint8_t);
		memcpy(packetPtr, curDest, destLen);
		packetPtr += destLen;
	}
	//copy text
	memcpy(packetPtr, arg, messageLength);
	packetPtr += messageLength;
	//packetComplete


	//send packet
	sent =  send(socketNum, packet, htons(cheader.packetLen), 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	//!!!!!
	//free 
	return 0;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle server-name server-port \n", argv[0]);
		exit(1);
	}
}
