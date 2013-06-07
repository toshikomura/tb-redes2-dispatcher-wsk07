#define MSG_A 0x0A
#define MSG_Y 0xF1
#define MSG_N 0x2B
#define MSG_B 0x45
#define MSG_X 0x3E
#define MSG_Z 0x12
#define MSG_C 0xDC
#define MSG_F 0x56
#define MSG_D 0xCD
#define MSG_G 0xE9
#define MSG_E 0xA8

#define MARK 0x7E

#define ERR_OPN_DIR 0
#define ERR_ARQ_BIG 1
#define ERR_ARQ_INX 2
#define ERR_ARQ_DIR 4

#define MAX_TAM_BUFFER 39

// Cabeçalho da mensagem
typedef struct mensg{
    unsigned char mark;
    unsigned char size:5;
    unsigned char seq:3;
    unsigned char type;
} header;

// estrutura de uma mensagem
typedef struct msg{
    header        hdr;
    char          *data;
    unsigned char par;
} msg;

msg *bufferToMsg(char *);

char *msgToBuffer(msg);

msg *createMsg(int, int, char *, int);

msg *createEmptyMsg(int, int);

void sendACK(int);

msg *sendAndWait(msg *, int, bool);

void sendError(int, int);
