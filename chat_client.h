
#define COMMAND_OFFSET 3
#define MAX_MSG_LEN 1000 //1000 cahracter max message
#define MAX_TEXT_LEN 200
#define MAX_HANDLE_LEN 251
#define STD_IN 0
#define MAXBUF 1024
#define DEBUG_FLAG 1
#define xstr(a) str(a)
#define str(a) #a

struct blockedHandles {
    char handle[MAX_HANDLE_LEN];
    struct blockedHandles *next;
};

int listRecieved(char *buf, struct chat_header cheader, int socketNum);
int exitServer(int socketNum);
int listHandles(int socketNum) ;
int printBlocked(struct blockedHandles *head);
int recievePacket(int socket, char *packet); 
int checkBlocked(char *srcHandle, struct blockedHandles *blockedHandles);
int unblock(char *textBuffer, struct blockedHandles **blockedHandles);
int block(char *textbuffer, struct blockedHandles **blockedHandles);
int printMessageText(char *packet);
int freeDestHandles(char **destHandles, int numDestinations);
int invalidDestRecieved(char *packet, struct chat_header cheader);
int messageRecieved(char *packet, struct chat_header cheader,  struct blockedHandles *blockedHandles);
void chatSession(int socketNum, char *srcHandle);
void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int serverActivity(int socketNum, struct blockedHandles *blockedHandles);
int localInput(int, struct blockedHandles **blockedHandles, char *srcHandle); 
int message(char *, int, char *);
int sendInitialPacket(int socketNum, char *srcHandle);
int sendPacket(char *packet, int socketNum, int packetLength);
