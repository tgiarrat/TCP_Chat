
#define MAX_MSG_LEN 1000 //1000 cahracter max message
#define MAX_HANDLE_LEN 250
#define STD_IN 0
#define MAXBUF 1024
#define DEBUG_FLAG 1
#define xstr(a) str(a)
#define str(a) #a

void chatSession(int socketNum);
void sendToServer(int socketNum);
void checkArgs(int argc, char * argv[]);
int localInput(); 
int message(char *);


struct init_packet{
	struct chat_header chatHeader;
	uint8_t handleLen;
	char *handle;
}__attribute__((packed));


struct chat_header{
	uint16_t packetLen;
	uint8_t byteFlag;
}__attribute__((packed));

struct message_packet{
	struct chat_header chatHeader;
	uint8_t handleLen;
	char *srcHandle;
	uint8_t numDestinations;
	//maybe i cant do the rest of this in a struct and ill kust need to make a biffer and memcpy
}__attribute__((packed));