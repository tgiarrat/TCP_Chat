#define MAXBUF 1024
#define DEBUG_FLAG 1
#define MAX_HANDLE_LEN 250




struct clientNode{
    char handle[MAX_HANDLE_LEN];
    int socket;
    struct clientNode *next;
}__attribute__((packed));

int sendPacket(int socket, char *packet, struct chat_header cheader); 
int getSocket(char *handle, struct clientNode *head);
int messageRecieved(char *recieved, struct chat_header cheader, struct clientNode *head);
int addClient(struct clientNode **head, char *handle, int handleLen, int clientSocket);
int newClientConnection(int serverSocket, struct clientNode **) ;
void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
int chatSession(int serverSocket, int portNumber);
int freeClientList(struct clientNode *head);
int checkHandle(char *handle, struct clientNode *head);
int sendHandleExistsError(int serverSocket);
int sendValidHandle(int socketServer);
int clientActivity(int clientSocket, struct clientNode *head) ;
