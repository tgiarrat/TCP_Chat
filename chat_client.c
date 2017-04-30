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
	struct blockedHandles *blockedHandles = NULL;
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
			serverActivity(socketNum, blockedHandles); //DONT NEED AND ????????? 

		}
		//keyboard update, read from keyboard
		if (FD_ISSET(0, &rfds)) { 
			localInput(socketNum, &blockedHandles);
			printf("$: ");
			fflush(stdout);
		}

	}

}

int serverActivity(int socketNum, struct blockedHandles *blockedHandles) {
	char buf[MAXBUF];
	int recieved, packetLength;
	uint8_t byteFlag;
	struct chat_header cheader;

	recievePacket(socketNum, buf);
	

	memcpy(&cheader, buf, sizeof(struct chat_header)); //gets the header from the recieved packet  
	byteFlag = cheader.byteFlag; 
	packetLength = ntohs(cheader.packetLen);

	//printf("Packet length recieved from server activity is %d and byte flag is: %d\n", packetLength, byteFlag);

	if (byteFlag == 5) {
		messageRecieved(buf + sizeof(struct chat_header), cheader, blockedHandles);
	}
	else if (byteFlag == 7) {
		//error: destination handle does not exist
		invalidDestRecieved(buf + sizeof(struct chat_header), cheader); 
		

	}
	printf("$:");
	fflush(stdout);
	return 0;
}

int invalidDestRecieved(char *packet, struct chat_header cheader){
	uint8_t handleSize; 
	char handle[MAX_HANDLE_LEN];

	memcpy(&handleSize, packet, sizeof(uint8_t));
	memcpy(handle, (packet + 1), handleSize);
	handle[handleSize] = '\0';
	printf("Client with handle %s does not exist\n", handle);
	return 0;
}

int messageRecieved(char *packet, struct chat_header cheader,  struct blockedHandles *blockedHandles) {
	char srcHandle[MAX_HANDLE_LEN];
	//char messageText[MAX_MSG_LEN];
	uint8_t srcHandleLen;
	int offset = 0;

	srcHandleLen = packet[offset++];
	memcpy(srcHandle, (packet + offset), srcHandleLen);
	srcHandle[srcHandleLen] = '\0';
	offset += srcHandleLen;

	if (checkBlocked(srcHandle, blockedHandles) == 1) {
		return 1;
	}
	printf("\n%s: ", srcHandle);
	printMessageText(packet+offset);
	return 0;
}

int checkBlocked(char *srcHandle, struct blockedHandles *blockedHandles){
	struct blockedHandles *curHandle = blockedHandles;

	while (curHandle != NULL) {
		if (strcmp(srcHandle, curHandle->handle) == 0) {
			return 1;
		}
		curHandle = curHandle->next; 
	}
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

int localInput(int socketNum, struct blockedHandles **blockedHandles) {
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
		printf("narrow\n");
		block(textBuffer + COMMAND_OFFSET, blockedHandles);
		printBlocked(*blockedHandles);
	}
	else if (commandType == 'U') { //unblock user
		unblock(textBuffer + COMMAND_OFFSET, blockedHandles);
	}
	else if (commandType == 'L') { //list handles
		listHandles(textBuffer);
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

int listHandles() {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader; 
	uint16_t packetSize = sizeof(struct chat_header);

	cheader.byteFlag = 10;
	cheader.packetLen = htons(packetSize);

	memcpy(packet, &cheader, packetSize);
	

}

int printBlocked(struct blockedHandles *head) {
	struct blockedHandles *curHandle;

	curHandle = head;
	printf("Blocked: ");
	while (curHandle != NULL) {
		printf("%s, ", curHandle->handle);
		curHandle = curHandle->next;
	}
	printf("\b\b  \n");

	return 0;
}

int unblock(char *textBuffer, struct blockedHandles **blockedHandles) {
	char *blockedHandle; 
	struct blockedHandles *curHandle;
	struct blockedHandles *temp;

	blockedHandle = strtok(textBuffer, " ");
	if ((checkBlocked(blockedHandle, *blockedHandles) == 0) || (*blockedHandles == NULL)) {
		printf("Unblock failed, handle %s is not blocked.\n", blockedHandle);
		return 1;
	}

	curHandle = *blockedHandles;
	if (strcmp(blockedHandle, curHandle->handle) == 0) {
		temp = curHandle;
		*blockedHandles = curHandle->next;
		free(temp);
		printf("Handle %s unblocked\n", blockedHandle);
		return 0;
	}
	while (curHandle->next != NULL) {
		if (strcmp(blockedHandle, curHandle->next->handle) == 0) {
			//printf("Got here\n");
			temp = curHandle->next;
			curHandle->next = curHandle->next->next;
			free(temp);
			return 0;
		}
		curHandle = curHandle->next;
	}
	return 0;
}

int block(char *textBuffer, struct blockedHandles **blockedHandles) {
	//
	//Ill admit the way I did blocking ended up being a little gross and badly done but.. oh well 
	//
	struct blockedHandles *newBlock = (struct blockedHandles *)malloc(sizeof(struct blockedHandles));
	struct blockedHandles *curHandle;
	char *invalidHandle;
	
	/*printf("\n");
	printf("Before blocking here is the list of handles:\n ");
	curHandle = *blockedHandles;
	while(curHandle != NULL) {
		printf("element: %s\n", curHandle->handle);				
		curHandle = curHandle->next; 
	}
	printf("\n");
	*/
	invalidHandle = strtok(textBuffer, " ");
	if(invalidHandle == NULL) {
		return 0;
	}
	printf("Here is the invalid handle... \n");
	if (checkBlocked(invalidHandle, *blockedHandles) == 1){
		printf("Block failed, handle %s is already blocked.\n", invalidHandle);
		return 1;
	}
	printf("\nstrlen of the new handle is %lu\n", strlen(invalidHandle));
	memcpy(newBlock->handle, invalidHandle, strlen(invalidHandle));
	newBlock->handle[strlen(invalidHandle)] = '\0'; 
	newBlock->next = NULL;
	if (*blockedHandles == NULL) {
		*blockedHandles =  newBlock;
		//printf("just once\n");
	}
	else {
		curHandle = *blockedHandles;
	
		while(curHandle->next != NULL) {
			if (checkBlocked(invalidHandle, *blockedHandles) == 1){
				printf("Block failed, handle %s is already blocked.\n", invalidHandle);
				return 1;
			}
			curHandle = curHandle->next;
		}
		curHandle->next = newBlock;
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
	packetPtr = packet;

	cheader.packetLen = htons(packetLength);
	cheader.byteFlag = 1;
	memcpy(packetPtr, &cheader, sizeof(struct chat_header));
	packetPtr += sizeof(struct chat_header);
	handleLen = strlen(handle);
	memcpy(packetPtr, &handleLen, sizeof(uint8_t)); //copy handle length
	packetPtr+= sizeof(uint8_t);
	memcpy(packetPtr, handle, handleLen); //copy handle

	sent = sendPacket(packet, socketNum, packetLength);
	//send packet
	//printf("Sending Initial packet...\n");
	/*sent =  send(socketNum, packet, packetLength, 0);
	if (sent < 0)
	{
		perror("flag = 1 send call");
		exit(-1);
	}
	if (sent == 0) {
		perror("sent 0 bytes instead of initial header packet");
		exit(-1);
	}*/

	recievePacket(socketNum,incomingBuffer);

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

int recievePacket(int socket, char *packet) {
	uint16_t packetLength;
	int messageLen;

	if ((messageLen = recv(socket, packet, sizeof(uint16_t), MSG_WAITALL)) < 2)
	{
		perror("RECV ERROR");
		exit(-1);
	}
	memcpy(&packetLength, packet, sizeof(uint16_t));
	packetLength = ntohs(packetLength);

	messageLen += recv(socket, packet + sizeof(uint16_t), packetLength - sizeof(uint16_t),MSG_WAITALL);
	if (messageLen < packetLength) {
		perror("error recieveing packet");
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

	sent = sendPacket(packet, socketNum, htons(cheader.packetLen));
	//send packet:
	/*sent =  send(socketNum, packet, htons(cheader.packetLen), 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}*/
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

int sendPacket(char *packet, int socketNum,int packetLength) {
	sent =  send(socketNum, packet, packetLength, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
	return 0;
}
