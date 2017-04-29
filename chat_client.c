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
	char *blockedHandles[MAX_HANDLE_LEN];
	sendInitialPacket(socketNum);
	printf("$: ");
	fflush(stdout);
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
			serverActivity(socketNum, blockedHandles);

		}
		//keyboard update, read from keyboard
		if (FD_ISSET(0, &rfds)) { 
			localInput(socketNum, blockedHandles);
			printf("$: ");
			fflush(stdout);
		}

	}

}

int serverActivity(int socketNum, char *blockedHandles[]) {
	char buf[MAXBUF];
	int recieved, packetLength;
	uint8_t byteFlag;
	struct chat_header cheader;

	if ((recieved = recv(socketNum, buf, MAXBUF, 0)) < 0) {
    	perror("Error recieveing incoming client packet \n");
    	exit(-1);
  	}
	if (recieved == 0) {
	   perror("Error: Read incoming server packet as zero bytes \n");
	   exit(-1);
   	}

	memcpy(&cheader, buf, sizeof(struct chat_header)); //gets the header from the recieved packet  
	byteFlag = cheader.byteFlag; 
	packetLength = ntohs(cheader.packetLen);

	//printf("Packet length recieved from server activity is %d and byte flag is: %d\n", packetLength, byteFlag);

	if (byteFlag == 5) {
		messageRecieved(buf + sizeof(struct chat_header), cheader);
	}
	else if (byteFlag == 7) {
		//error: destination handle does not exist
		invalidDestRecieved(buf + sizeof(struct chat_header), cheader); 
		

	}
	printf("\n$:");
	return 0;
}

int invalidDestRecieved(char *packet, struct chat_header cheader){
	return 0;
}

int messageRecieved(char *packet, struct chat_header cheader) {
	char srcHandle[MAX_HANDLE_LEN];
	//char messageText[MAX_MSG_LEN];
	uint8_t srcHandleLen;
	int offset = 0;

	srcHandleLen = packet[offset++];
	memcpy(srcHandle, (packet + offset), srcHandleLen);
	srcHandle[srcHandleLen] = '\0';
	offset += srcHandleLen;
	
	printf("\n%s: ", srcHandle);
	printMessageText(packet+offset);

	//printf("Sending client handle length: %d    Sending Client handle: %s\n ", srcHandleLen, handle); 
	return 0;


}

int printMessageText(char *packet) {
	int offset = 0; 
	int messageLen, i;
	uint8_t numDest;
	uint8_t curHandleLen;

	numDest = *(packet + offset++);
	//printf("offset is: %d   num dest is:  %d\n", offset, numDest);
	for (i = 0; i < numDest; i++) {
		curHandleLen = *(packet + offset++);
		//printf("Cur handle length is %d\n", curHandleLen);
		offset+= curHandleLen;
	}
	//offset should now be pointing to the start of the message
	//messageLen = cheader.packetLen - offset;
	//printf("offset is: %d\n", offset);
	//printf("Here is the message : %s\n", packet + offset);
	printf("%s\n", packet + offset);
	return 0;
}

int localInput(int socketNum, char *blockedHandles[]) {
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
		block(textBuffer + COMMAND_OFFSET, blockedHandles);
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

int block(char *textBuffer, char *blockedHandles[]) {
	char *curHandle;
	int i = 0;
	
	curHandle = blockedHandles[i];
	while(curHandle != NULL) {
		printf("%dst handle in blocked list is: %s", i , curHandle);
		curHandle = blockedHandles[i];
		i++;
	}
	curHandle = malloc(MAX_HANDLE_LEN);
	strcpy(curHandle, textBuffer);
	
	
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
	//printf("\nInitial packet size is: %d\n", packetLength);
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
	//printf("Sending Initial packet...\n");
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
	//printf("Recieveing packet response after sending intitial packet...\n");
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
		perror("Invalid Handle Flag = 3\n");
		exit(-1);
	}
	else if (flag ==2) {
		//printf("Valid handle Flag = 2\n");
	}
	else {
		perror("uh oh... flag != 2 or 3\n");
		exit(-1);
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

	//Get destination handles:
	arg = strtok(textBuffer, " "); //get a space separated token of the string 
	if (isdigit(textBuffer[0])) {
		numDestinations = atoi(arg);
		arg = strtok(NULL, " ");
	}
	else {
		numDestinations = 1;
	}
	destHandles = malloc(numDestinations);
	for (i = 0; (i < numDestinations); i++) {

		*(destHandles + i) = malloc(strlen(arg));
		memcpy( (*(destHandles+i)) , arg, strlen(arg));
		destHandleTotal += strlen(arg);
		if (i != numDestinations - 1) { //dont get the next token
			arg = strtok(NULL, " ");
		}
	}
	if (i != numDestinations){
		printf("Error: Incorrect number of destination handles entered\n");
		return -1;
	}

	arg = strtok (NULL, "\n"); //arg is message
	messageLength = strlen(arg) + 1;  // plus one is for the null terminating character at the end	
	cheader.byteFlag = 5;
	srcLength = strlen(handle);
	cheader.packetLen =
		htons(sizeof(struct chat_header) + srcLength + messageLength 
		+ numDestinations + 2 + destHandleTotal);
	/*printf("\n");
	printf("-----------PRINTING THE PACKET INFO---------------------\n");
	printf("text is: %s\n", arg);
	printf("Packet Length is: %d\n", ntohs(cheader.packetLen));
	printf("Handle is: %s\n", handle);
	printf("first dest is: %s\n", *destHandles);
	printf("second dest is: %s\n", *(destHandles + 1));	
	printf("\n");*/

	//Make packet:
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


	//send packet:
	sent =  send(socketNum, packet, htons(cheader.packetLen), 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
	freeDestHandles(destHandles, numDestinations); 
	return 0;
}

int freeDestHandles(char **destHandles, int numDestinations) {
	//char *curHandlee;
	int i;

	for (i = 0; i < numDestinations; i++) {
		free(destHandles[i]);
	}
	free(destHandles);
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
