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

#include "networks.h"
#include "chat_client.h"

char handle[MAX_HANDLE_LEN];
int socketNum;
//int seq;



int main(int argc, char * argv[])
{

	int socketNum = 0;         //socket descriptor, will be the socket of the server i believe

	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	
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
			localInput();
		}

	}

}

int localInput() {
	char textBuffer[MAXBUF];
	int textLength;
	char commandType;
	
    //while ((textBuffer[textLength] = getchar()) != '\n' && textLength < MAX_MSG_LEN){
    //  textLength++;
	//}
	fgets(textBuffer, MAXBUF, stdin);
	textBuffer[strcspn(textBuffer, "\n")] = 0;
	printf("%s last char: %c", textBuffer, textBuffer[strcspn(textBuffer, "\0")-1]);

	//textBufffer[textLength] = '\0'; // gets rid of the \n and ends the string

	if (textBuffer[0] != '%') {
		printf("Incorrect message formatting\n");
		return -1;
	}
	
	commandType = toupper(textBuffer[1]); //letter of the command will be here in a properly formatted message

	if (commandType == 'M') { //send message
		//message(textBuffer);
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
