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

int message(char *textBuffer, int socketNum) {
	char *packet; //this will be the entire packet. I knoe its a lil confusing but oh well too late
	char *packetPtr;
	char *arg;
	char **destHandles;
	char *curDest;
	struct chat_header cheader;
	uint8_t srcLength, numDestinations;
	int messageLength, i, destLen, sent;
	int destHandleTotal = 0;

	//
	cheader.byteFlag = 5;
	srcLength = strlen(handle);
	arg = strtok(textBuffer, " "); //get a space separated token of the string 
	numDestinations = atoi(arg);
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

	arg = strtok (NULL, " "); //arg is message
	messageLength = strlen(arg);
	//strcpy(,arg,messageLength); //COPY THE MESSAGE


	cheader.packetLen =
		htons(sizeof(struct chat_header) + srcLength + messageLength 
		+ numDestinations + 2 + destHandleTotal);

	printf("\n");
	printf("Packet Length is: %d\n", ntohs(cheader.packetLen));
	//memcpy(messagePacket.srcHandle, handle, messagePacket.srcLen); //PROBLEM HERE
	printf("Handle is: %s\n", handle);
	printf("first dest is: %s\n", *destHandles);
	printf("second dest is: %s\n", *(destHandles + 1));
	printf("\n");	

	//begin making packet:
	packet = malloc(ntohs(cheader.packetLen)); 
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
	memcpy(packetPtr, arg, strlen(arg));
	packetPtr += strlen(arg);
	//!!!!!!!!!!!!!!!!!!
	//NO SEGFAULT UP TO HERE CONFIRMED 
	//!!!!!!!!!!!!!!!!!!

	//send packet

	sent =  send(socketNum, packet, cheader.packetLen, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	return 0;
}


void sendToServer(int socketNum)
{
	char sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
			
	printf("Enter the data to send:\n");
	scanf("%" xstr(MAXBUF) "[^\n]%*[^\n]", sendBuf);
	
	sendLen = strlen(sendBuf) + 1;
	printf("read: %s len: %d\n", sendBuf, sendLen);
		
	sent =  send(socketNum, sendBuf, sendLen, 0);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("String sent: %s \n", sendBuf);
	printf("Amount of data sent is: %d\n", sent);
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
