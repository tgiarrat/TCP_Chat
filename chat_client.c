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

int main(int argc, char * argv[])
{
	int socketNum = 0; 
	char srcHandle[MAX_HANDLE_LEN];
	checkArgs(argc, argv);
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	strcpy(srcHandle, argv[1]);
	chatSession(socketNum, srcHandle);
	close(socketNum);
	return 0;
}

void chatSession(int socketNum, char *srcHandle) {
	int connected = 1;
	fd_set rfds;
	struct blockedHandles *blockedHandles = NULL;
	sendInitialPacket(socketNum, srcHandle);
	printf("$: ");
	fflush(stdout);
	while (connected) { //1 is possibly temporary, need to run client until the user exits the client
		
		FD_ZERO(&rfds);
		FD_SET(STD_IN, &rfds); //watch std in
		FD_SET(socketNum, &rfds ); //watch socket for update
		
		if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
        	perror("select call error\n");
        	exit(-1);
      	}	
		
		//server update, read from server
		if (FD_ISSET(socketNum, &rfds)) {
			connected = serverActivity(socketNum, blockedHandles); 
		}
		//keyboard update, read from keyboard
		else if (FD_ISSET(0, &rfds)) { 
			localInput(socketNum, &blockedHandles, srcHandle);
			printf("$: ");
			fflush(stdout);
		}

	}

}

int serverActivity(int socketNum, struct blockedHandles *blockedHandles) {
	char buf[MAX_PACKET_SIZE];
	uint8_t byteFlag;
	struct chat_header cheader;
	uint8_t blocked = 0; //ok this is a really lazy way but cut me some slack

	recievePacket(socketNum, buf);
	

	memcpy(&cheader, buf, sizeof(struct chat_header)); //gets the header from the recieved packet  
	byteFlag = cheader.byteFlag; 


	if (byteFlag == 5) {
		blocked = messageRecieved(buf + sizeof(struct chat_header), cheader, blockedHandles);
	}
	else if (byteFlag == 7) {
		//error: destination handle does not exist
		invalidDestRecieved(buf + sizeof(struct chat_header), cheader); 
	}
	else if (byteFlag == 11) {
		//listing handles
		listRecieved(buf + sizeof(struct chat_header), cheader, socketNum);
	}
	else if(byteFlag == 9) {
		//printf("got here at least\n");
		exitACK(blockedHandles);
		return 0;
	}
	if(blocked == 0) {
		printf("$:");
		fflush(stdout);
	}
	return 1;
}

int exitACK (struct blockedHandles *head) {
	struct blockedHandles *curHandle;
	struct blockedHandles *temp; 


	curHandle = head;
	while (curHandle != NULL) {
		temp = curHandle;
		curHandle = curHandle->next; 
		free(temp);
	}


	return 0;
}



int listRecieved(char *buf,struct chat_header cheader, int socketNum) {
	char packet[MAX_PACKET_SIZE];
	uint32_t handleCount;
	char curHandle[MAX_HANDLE_LEN];
	uint8_t curHandleLen; 

	memcpy(&handleCount, buf, sizeof(uint32_t));
	handleCount = ntohl(handleCount);
	printf("\nNumber of clients: %lu\n", (unsigned long)handleCount);
	
	recievePacket(socketNum,packet);
	memcpy(&cheader, packet, sizeof(struct chat_header));
	while (cheader.byteFlag != 13) {
		memcpy(&curHandleLen, packet + sizeof(struct chat_header), sizeof(uint8_t));
		memcpy(curHandle, packet + sizeof(struct chat_header) + sizeof(uint8_t), curHandleLen);
		curHandle[curHandleLen] = '\0';
		printf("   %s\n", curHandle);
		recievePacket(socketNum, packet);
		memcpy(&cheader, packet, sizeof(struct chat_header));
	}
	return 0;
}

int invalidDestRecieved(char *packet, struct chat_header cheader){
	uint8_t handleSize; 
	char invalidhandle[MAX_HANDLE_LEN];

	memcpy(&handleSize, packet, sizeof(uint8_t));
	memcpy(invalidhandle, (packet + 1), handleSize);
	invalidhandle[handleSize] = '\0';
	printf("Client with handle %s does not exist\n", invalidhandle);
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
	int i;
	uint8_t numDest;
	uint8_t curHandleLen;
	numDest = *(packet + offset++);
	for (i = 0; i < numDest; i++) {
		curHandleLen = *(packet + offset++);
		offset+= curHandleLen;
	}
	printf("%s\n", packet + offset);
	return 0;
}

int localInput(int socketNum, struct blockedHandles **blockedHandles, char *srcHandle) {
	char textBuffer[MAXBUF];
	char commandType;
	
	fgets(textBuffer, MAXBUF, stdin);
	textBuffer[strcspn(textBuffer, "\n")] = 0;
	if (textBuffer[0] != '%') {
		printf("Incorrect message formatting\n");
		return -1;
	}

	commandType = toupper(textBuffer[1]); //letter of the command will be here in a properly formatted message
	if (strlen(textBuffer) > MAX_MSG_LEN) {
		printf("Command entered is too long, max command length is 1000\n");
		return 1;
	}

	if (commandType == 'M') { //send message
		message(textBuffer + COMMAND_OFFSET, socketNum, srcHandle);
	}
	else if (commandType == 'B') { //block user
		block(textBuffer + COMMAND_OFFSET, blockedHandles);
		printBlocked(*blockedHandles);
	}
	else if (commandType == 'U') { //unblock user
		unblock(textBuffer + COMMAND_OFFSET, blockedHandles);
	}
	else if (commandType == 'L') { //list handles
		listHandles(socketNum);
	}
	else if (commandType == 'E') { //exit
		exitServer(socketNum);
	}
	else {
		printf("Error: %c is an invalid command type\n", commandType);
		return -1;
	}

	return 0;

}

int exitServer(int socketNum) {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader;

	cheader.byteFlag = 8;
	cheader.packetLen = htons(sizeof(struct chat_header));
	memcpy(packet, &cheader, sizeof(struct chat_header));
	sendPacket(packet, socketNum, sizeof(struct chat_header));

	return 0;
}

int listHandles(int socketNum) {
	char packet[MAX_PACKET_SIZE];
	struct chat_header cheader; 
	uint16_t packetSize = sizeof(struct chat_header);

	cheader.byteFlag = 10;
	cheader.packetLen = htons(packetSize);
	memcpy(packet, &cheader, packetSize);
	
	sendPacket(packet, socketNum, packetSize);
	return 0;
}

int printBlocked(struct blockedHandles *head) {
	struct blockedHandles *curHandle;

	curHandle = head;
	printf("Blocked:  ");
	while (curHandle != NULL) {
		printf("%s, ", curHandle->handle);
		curHandle = curHandle->next;
	}
	printf("\b\b  \n\n");
	return 0;
}

int unblock(char *textBuffer, struct blockedHandles **blockedHandles) {
	char *blockedHandle; 
	struct blockedHandles *curHandle;
	struct blockedHandles *temp;

	printf("\n");
	blockedHandle = strtok(textBuffer, " ");
	if (blockedHandle == NULL) { //no handle provieded check
		printf("Ublock failed, no handle provided\n\n");
		return 1;
	}
	if ((checkBlocked(blockedHandle, *blockedHandles) == 0) || (*blockedHandles == NULL)) {
		printf("Unblock failed, handle %s is not blocked.\n\n", blockedHandle);
		return 1;
	}
	curHandle = *blockedHandles;
	if (strcmp(blockedHandle, curHandle->handle) == 0) {
		temp = curHandle;
		*blockedHandles = curHandle->next;
		free(temp);
		printf("Handle %s unblocked\n\n", blockedHandle);
		return 0;
	}
	while (curHandle->next != NULL) {
		if (strcmp(blockedHandle, curHandle->next->handle) == 0) {
			//printf("Got here\n");
			temp = curHandle->next;
			curHandle->next = curHandle->next->next;
			printf("Handle %s unblocked\n\n", blockedHandle);
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
	printf("\n");
	invalidHandle = strtok(textBuffer, " ");
	if(invalidHandle == NULL) {
		return 0;
	}
	if (checkBlocked(invalidHandle, *blockedHandles) == 1){
		printf("Block failed, handle %s is already blocked.\n", invalidHandle);
		return 1;
	}
	memcpy(newBlock->handle, invalidHandle, strlen(invalidHandle));
	newBlock->handle[strlen(invalidHandle)] = '\0'; 
	newBlock->next = NULL;
	if (*blockedHandles == NULL) {
		*blockedHandles =  newBlock;
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

int sendInitialPacket(int socketNum, char *srcHandle){
	char packet[MAX_PACKET_SIZE];
	char *packetPtr;
	int packetLength, handleLen;
	struct chat_header cheader;
	char incomingBuffer[MAXBUF];
	uint8_t flag;

	handleLen = strlen(srcHandle);
	packetLength = sizeof(struct chat_header) + 1 + handleLen;
	packetPtr = packet;

	cheader.packetLen = htons(packetLength);
	cheader.byteFlag = 1;
	memcpy(packetPtr, &cheader, sizeof(struct chat_header));
	packetPtr += sizeof(struct chat_header);
	memcpy(packetPtr, &handleLen, sizeof(uint8_t)); //copy handle length
	packetPtr+= sizeof(uint8_t);
	memcpy(packetPtr, srcHandle, handleLen); //copy handle

	sendPacket(packet, socketNum, packetLength);
	recievePacket(socketNum,incomingBuffer);

	memcpy(&flag, incomingBuffer + sizeof(uint16_t), sizeof(uint8_t)); //gets the flag from the incoming buffer
	if (flag == 3) {
		printf("Handle already in use: %s\n", srcHandle);
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

int message(char *textBuffer, int socketNum, char *srcHandle) {
	char packet[MAX_PACKET_SIZE]; //this will be the entire packet. I knoe its a lil confusing but oh well too late
	char *packetPtr;
	char *arg;
	char textSegment[MAX_TEXT_LEN];
	char **destHandles;
	char *curDest;
	struct chat_header cheader;
	uint8_t srcLength, numDestinations ;
	int messageLength, i, destLen;
	int destHandleTotal = 0;
	int textSize;

	//Get destination handles:
	arg = strtok(textBuffer, " "); //get a space separated token of the string 
	if (arg == NULL) {
		printf("Invalid Command \n");
		return 1; 
	}
	if (textBuffer[0] == ' ') {
		printf("Extra spaces, format correctly >:(\n");
		return 1;
	}
	if (isdigit(textBuffer[0])) {
		numDestinations = atoi(arg);
		if (numDestinations > 9) {
			printf("Your number of message recipients desired is %d larger than allowed\n", (numDestinations - 9));
			return 1;
		}
		arg = strtok(NULL, " ");
	}
	else {
		numDestinations = 1;
	}
	destHandles = malloc(numDestinations);
	for (i = 0; (i < numDestinations); i++) {
		if (strlen(arg) >= MAX_HANDLE_LEN) {
			printf("A destination handle entered is too large \n");
			freeDestHandles(destHandles, (i+1)); 
			return 1;
		}
		*(destHandles + i) = calloc(1, strlen(arg));
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
	if (arg == NULL) {
		arg = "";
	}
	
	messageLength = strlen(arg) + 1;  // plus one is for the null terminating character at the end	

	cheader.byteFlag = 5;
	srcLength = strlen(srcHandle);

	while (messageLength > 1) {
		if (messageLength > 200)  {
			textSize = MAX_TEXT_LEN;//minus 1 for the mull char
			memcpy(textSegment, arg , textSize-1);
			textSegment[MAX_TEXT_LEN] = '\0';
		}
		else {
			textSize = messageLength;
			memcpy(textSegment, arg, textSize);
		}
		cheader.packetLen =
			htons(sizeof(struct chat_header) + srcLength + textSize 
			+ numDestinations + 2 + destHandleTotal);

		packetPtr = packet;
		memcpy(packetPtr, &cheader, sizeof(struct chat_header)); //copy chat header
		packetPtr += sizeof(struct chat_header);
		memcpy(packetPtr, &srcLength, sizeof(uint8_t));
		packetPtr += sizeof(uint8_t);
		memcpy(packetPtr, srcHandle, srcLength);
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
		memcpy(packetPtr, textSegment, textSize);
		packetPtr += textSize;
		//packetComplete

		sendPacket(packet, socketNum, ntohs(cheader.packetLen));


		messageLength -= (textSize - 1);
		arg += (textSize - 1);
	}
	freeDestHandles(destHandles, numDestinations); 
	return 0;
}

int freeDestHandles(char **destHandles, int numDestinations) {
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
	if (strlen(argv[1]) >= MAX_HANDLE_LEN) {
		printf("Desired handle is %lu characters greater than the maximum handle size.\n", (unsigned long)((strlen(argv[1]) - MAX_HANDLE_LEN) + 1));
		exit(1);
	}
}

int sendPacket(char *packet, int socketNum,int packetLength) {
	int sent;
	
	sent =  send(socketNum, packet, packetLength, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}
	return sent;
}
