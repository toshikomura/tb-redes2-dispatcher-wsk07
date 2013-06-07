#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <unistd.h>

#include "protocol.h"
#include "socket.h"

using namespace std;

msg *bufferToMsg(char *buffer){
    msg *m = (msg *) malloc(sizeof(msg));

    memcpy(&(m->hdr), buffer, sizeof(header));

    if ( m->hdr.mark != MARK )
        return (msg *) -1;

    m->data = (char *)malloc(m->hdr.size);
    memcpy(m->data,   buffer+sizeof(header),  m->hdr.size);
    memcpy(&(m->par), buffer+sizeof(header)+m->hdr.size, 1);

    return m;
}

char *msgToBuffer(msg m){
    char *b = (char *) malloc(MAX_TAM_BUFFER);

    memcpy(b, &(m.hdr),  sizeof(header));
    memcpy(b+sizeof(header), m.data,    m.hdr.size);
    memcpy(b+sizeof(header)+m.hdr.size, &(m.par),  sizeof(m.par));

    return b;
}

msg *createMsg(int seq, int type, char *data, int size){
    msg *m;

    m = (msg *) malloc(sizeof(msg));

    m->hdr.mark = MARK;
    m->hdr.size = size;
    m->hdr.seq  = seq;
    m->hdr.type = type;

    m->data = (char *) data;
    m->par  = 1;

    return m;
}

msg *createEmptyMsg(int seq, int type){
    msg *m;

    m = (msg *) malloc(sizeof(msg));

    m->hdr.mark = MARK;
    m->hdr.size = 0;
    m->hdr.seq  = seq;
    m->hdr.type = type;

    m->data = NULL;
    m->par  = 1;

    return m;
}

void sendACK(int sckt){
    msg  *m;
    char *buffer;

    m      = createEmptyMsg(0, MSG_Y);
    buffer = msgToBuffer(*m);

    sendAndWait(m, sckt, false);
}

msg *sendAndWait(msg *m, int sckt, bool wait){
    int  s, r, err;
    char *buffer;
    msg  *m1 = (msg *) malloc(sizeof(msg));

    buffer = msgToBuffer(*m);
    s      = send(sckt, buffer, (ssize_t) MAX_TAM_BUFFER, 0);

    if ( !wait )
        return (msg *) 0;

    while (true){
        r = recv(sckt, buffer, (ssize_t) MAX_TAM_BUFFER, 0);

        m1 = bufferToMsg(buffer);
        if ( (int) m1 == -1 )
            continue;

        if ( m1->hdr.type == MSG_Y )
            break;

        if ( m1->hdr.type == MSG_E ){
            err = atoi(m->data);
            if ( err == 0 )
                error((char *) "Diretório inexistente\n", false);
            else if ( err == 2 )
                error((char *) "Arquivo inexistente\n", false);
            break;
        }

        if ( m1->hdr.type == MSG_X || m1->hdr.type == MSG_F ||
             m1->hdr.type == MSG_Z)
            break;
    }

    return m1;
}

void sendError(int sckt, int err){
    char data[5], *buffer;
    int size, s;
    msg *m = (msg *) malloc(sizeof(msg));

    sprintf(data, "%d", err);

    size   = strlen(data) + 1;
    m      = createMsg(0, MSG_E, data, size);
    buffer = msgToBuffer(*m);
    s      = send(sckt, buffer, (ssize_t) MAX_TAM_BUFFER, 0);
}
