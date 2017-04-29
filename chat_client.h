
#define COMMAND_OFFSET 3
#define MAX_MSG_LEN 1000 //1000 cahracter max message
#define MAX_HANDLE_LEN 251
#define STD_IN 0
#define MAXBUF 1024
#define DEBUG_FLAG 1
#define xstr(a) str(a)
#define str(a) #a

int block(char *textbuffer, char *blockedHandles[]);
int printMessageText(char *packet);
int freeDestHandles(char **destHandles, int numDestinations);
int invalidDestRecieved(char *packet, struct chat_header cheader);
int messageRecieved(char *packet, struct chat_header cheader);
void chatSession(int socketNum);
void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int serverActivity(int socketNum, char *blockedHandles[]);
int localInput(int, char *blockedHandles[]); 
int message(char *, int);
int sendInitialPacket(int socketNum);

