#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <cstring>
#include <dirent.h>

#include "socket.h"
#include "protocol.h"

using namespace std;

int  server;
char *path, *pathAnt;
DIR  *dir;

void changeDirectory(){
    int  s, size;
    char *buffer;
    msg  *m = (msg *) malloc(sizeof(msg));

    dir = opendir(path);

    if ( dir == NULL ){
        sendError(server, ERR_OPN_DIR);
        strcpy(path, pathAnt);
    } else
        sendACK(server);
}

void myLs(){
    int    size, s;
    struct dirent *de;
    struct stat   buf;
    msg    *m;
    char   *buffer;

    dir = opendir(path);
    while ( de = readdir(dir) ){
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        // stat(path,&buf);

        size = strlen(de->d_name);
        if ( de->d_name[0] != '.' ){
            m = createMsg(0, MSG_X, de->d_name, size);
            sendAndWait(m, server, true);
        }
    }

    m      = createEmptyMsg(0, MSG_Z);
    buffer = msgToBuffer(*m);
    s      = send(server, buffer, (ssize_t) MAX_TAM_BUFFER, 0);
}

void get(char *p){
    int    size, s;
    msg    *m;
    struct stat buf;
    char   *buffer = (char *) malloc(64*sizeof(char));
    FILE   *f;

    printf("path no get: %s\n", p);

    f = fopen(p, "rb");
    if ( f == NULL ){
        sendError(server, ERR_ARQ_INX);
        return;
    }

    stat(p, &buf);
    if ( S_ISREG(buf.st_mode) ){
        char data[5];
        sprintf(data, "%d", (int) buf.st_size);

        size = strlen(data) + 1;
        m    = createMsg(0, MSG_F, data, size);
        printf("server vai enviar f\n");
        printf("tam ar: %d\n", atoi(m->data));
        sendAndWait(m, server, true);
        printf("server enviou f\n");

        while ( (size = fread(buffer, 1, 31*sizeof(char), f)) != 0){
            printf("size: %d buffer server: %s\n", size, buffer);
            m = createMsg(0, MSG_D, buffer, size);
            printf("server vai enviar d\n");
            sendAndWait(m, server, true);
            printf("server enviou d\n");
        }

        m = createEmptyMsg(0, MSG_Z);
        printf("server vai enviar z\n");
        sendAndWait(m, server, true);
        printf("server enviou z\n");
    } else {
        sendError(server, ERR_ARQ_DIR);
        strcpy(path, pathAnt);
    }

    fclose(f);
}

int main(){
    int  r;
    char *pathAux, buffer[45];
    msg *m;

    server = createSocket((char *) "eth0");

    path    = (char *)malloc(99999999*sizeof(char));
    pathAnt = (char *)malloc(99999999*sizeof(char));
    pathAux = (char *)malloc(99999999*sizeof(char));
    path[0] = '.';
    path[1] = '/';
    path[2] = '\0';

    while (true){
        r = recv(server, buffer, (ssize_t) MAX_TAM_BUFFER, 0);

        if ( r != MAX_TAM_BUFFER )
            error((char *) "Socket erorr", true);

        m = bufferToMsg(buffer);
        if ( (int) m == -1 )
            continue;

        switch (m->hdr.type){
            case MSG_A: printf("cd\n");
                        strcpy(pathAnt, path);
                        if ( m->data[0] == '/' )
                            strcpy(path, m->data);
                        else {
                            strcat(path, m->data);
                            if ( path[strlen(path)-1] != '/' )
                                strcat(path, "/");
                        }
                        changeDirectory();
                        break;
            case MSG_B: printf("ls\n");
                        myLs();
                        break;
            case MSG_Z: printf("fim\n");
                        break;
            case MSG_C: printf("put\n");
                        printf("%s\n", m->data);
                        break;
            case MSG_F: printf("tamanho do arquivo em bytes\n");
                        break;
            case MSG_D: printf("dados\n");
                        break;
            case MSG_G: printf("get\n");
                        printf("strcpy path: %s", path);
                        strcpy(pathAux, path);
                        strcat(pathAux, m->data);
                        printf("m->data: %s\n", m->data);
                        printf("path: %s\n", pathAux);
                        get(pathAux);
                        break;
        }
    }

    return 0;
}
