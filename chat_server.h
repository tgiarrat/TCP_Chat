
#include "networks.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1
#define MAX_HANDLE_LEN 251




struct clientNode{
    char handle[MAX_HANDLE_LEN];
    int socket;
    struct clientNode *next;
}__attribute__((packed));

int removeClientNode(struct clientNode **head, int socket);
int clientExit(struct clientNode **head ,int clientSocket);
int listHandles(struct clientNode *head, int socket);
int recievePacket(int socket, char *packet);
int sendPacket(int socket, char *packet, struct chat_header cheader); 
int getSocket(char *handle, struct clientNode *head);
int sendInvalidDest(int destSocket ,int sendingSocket, char *destHandle, uint8_t destHandleLength);
int messageRecieved(char *recieved, struct chat_header cheader, struct clientNode *head, int sendingSocket);
int addClient(struct clientNode **head, char *handle, int handleLen, int clientSocket);
int newClientConnection(int serverSocket, struct clientNode **) ;
void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
int chatSession(int serverSocket, int portNumber);
//int freeClientList(struct clientNode *head);
int checkHandle(char *handle, struct clientNode *head);
int sendHandleExistsError(int serverSocket);
int sendValidHandle(int socketServer);
int clientActivity(int clientSocket, struct clientNode **head) ;
