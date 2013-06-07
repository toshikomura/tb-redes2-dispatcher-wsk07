#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
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

int  client;
char *path, *pathAnt, buffer[MAX_TAM_BUFFER];

int findComand(char *c){
    if ( c[0] == 'l' && c[1] == 's' && (c[2] == '\0' || c[2] == ' ') )
        return 1;

    if ( c[0] == 'l' && c[1] == 'l' && c[2] == 's' && (c[3] == '\0'
         || c[4] == ' ') )
        return 2;

    if ( c[0] == 'c' && c[1] == 'd' && c[2] == ' ' )
        return 3;

    if ( c[0] == 'l' && c[1] == 'c' && c[2] == 'd' && c[3] == ' ' )
        return 4;

    if ( c[0] == 'g' && c[1] == 'e' && c[2] == 't' && c[3] == ' ')
        return 5;

    if ( c[0] == 'p' && c[1] == 'u' && c[2] == 't' && c[3] == ' ')
        return 6;

}

char *findPath(char *cmd, int ndx){
    int  i=0, size;
    char *c;

    size = strlen(&cmd[ndx+1]) + 1;
    if ( size > 31 ){
        error((char *) "Caminho passado no comando é muito grande!", false);
        error((char *) "Tamanho máximo aceito: 31 bytes!", false);
        return NULL;
    }

    c = (char *) malloc(sizeof(char)*size);
    memcpy(c, cmd + ndx+1, sizeof(char)*size);

    return c;
}

void rcvLs(msg *m){
    int r, count=1;
    msg *m1;

    for ( int i=0; i<m->hdr.size; i++)
        printf("%c", m->data[i]);
    printf("\n");

    sendACK(client);

    while ( true ){
        r = recv(client, buffer, (ssize_t) MAX_TAM_BUFFER, 0);

        if ( r != MAX_TAM_BUFFER )
            error((char *) "Socket error", true);

        m1 = bufferToMsg(buffer);
        if ( (int) m1 == -1 )
            continue;

        if ( m1->hdr.type == MSG_X ){
            for ( int i=0; i<m1->hdr.size; i++)
                printf("%c", m1->data[i]);
            printf("\n");
            count++;
            sendACK(client);
        }

        if ( m1->hdr.type == MSG_Z )
            break;
    }

    printf("total %d\n", count);

    sendACK(client);
}

void lsLocal(){
    int    size, count=0;
    struct dirent *de;
    struct stat   buf;

    DIR *dir = opendir(path);
    while ( de = readdir(dir) ){
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        size = strlen(de->d_name);
        if ( de->d_name[0] != '.' ){
            for ( int i=0; i<size; i++)
                printf("%c", de->d_name[i]);
            printf("\n");
            count++;
        }
    }

    printf("total %d\n", count);

}

void cdLocal(){
    if ( path[0] != '/' )
        strcat(path, "./");

    DIR *dir = opendir(path);

    if ( dir == NULL ){
        error((char *) "Diretório inexistente", false);
        strcpy(path, pathAnt);
    }
}

void rcvGet(msg *m, char *p){
    int  r, size, seq=0;
    msg  *m1 = (msg *) malloc(sizeof(msg));
    FILE *f;

    f = fopen(p, "wb");
    if ( f == NULL ){
        error((char *) "Erro ao abrir arquivo!", false);
        m = createEmptyMsg(0, MSG_N);
        sendAndWait(m, client, false);
    }

    size = strlen(p) + 1;
    m    = createMsg(seq, MSG_G, p, size);

    printf("esperando primeira vez\n");
    m    = sendAndWait(m, client, true);
    printf("cliente recebe ack\n");

    printf("tam arq: %d\n", atoi(m->data));

    if ( m->hdr.type != MSG_F )
        return;

    if ( true /*m->hdr.size cabe aki*/ )
        sendACK(client);
    else
        sendError(client, ERR_ARQ_BIG);

    while ( true ){
        r = recv(client, buffer, (ssize_t) MAX_TAM_BUFFER, 0);

        if ( r != MAX_TAM_BUFFER )
            error((char *) "Socket error", true);

        m1 = bufferToMsg(buffer);
        if ( (int) m1 == -1 )
            continue;

        printf("cliente recebe d\n");
        if ( m1->hdr.type == MSG_D ){
            printf("size: %d buffer cliente: %s\n", m1->hdr.size,m1->data);
            fwrite(m1->data, 1, m1->hdr.size*sizeof(char), f);
            sendACK(client);
            printf("cliente escreveu enviou ack server\n");
        }

        if ( m1->hdr.type == MSG_Z ){
            sendACK(client);
            break;
        }
    }

    fclose(f);

}

int main(){
    int  seq=0, cmnd, size;
    char *pathAux, *pathAux2, command[99999];
    msg  *m, *err;

    m      = (msg *) malloc(sizeof(msg));
    client = createSocket((char *) "eth0");

    path     = (char *)malloc(999999*sizeof(char));
    pathAnt  = (char *)malloc(999999*sizeof(char));
    path[0] = '.';
    path[1] = '/';
    path[2] = '\0';

    while (true){

        printf("ftp> ");

        if ( scanf(" %[^\n]", command) == EOF ){
            printf("Ate logo!\n");
            break;
        }

        cmnd = findComand(command);

        switch (cmnd){
            case 1: printf("ls\n");
                    m = createEmptyMsg(seq, MSG_B);
                    m = sendAndWait(m, client, true);
                    if ( m->hdr.type == MSG_X )
                        rcvLs(m);
                    if ( m->hdr.type == MSG_Z )
                        printf("total 0\n");
                    break;
            case 2: printf("lls\n");
                    lsLocal();
                    break;
            case 3: printf("cd\n");
                    pathAux2 = findPath(command, 2);
                    if ( pathAux2 == NULL )
                        continue;
                    size = strlen(pathAux2) + 1;
                    m    = createMsg(seq, MSG_A, pathAux2, size);
                    sendAndWait(m, client, true);
                    break;
            case 4: printf("lcd\n");
                    pathAux = findPath(command, 3);
                    strcpy(pathAnt, pathAux);
                    if ( pathAux[0] == '/' )
                        strcpy(path, pathAux);
                    else {
                        strcat(path, "/");
                        strcat(path, pathAux);
                    }
                    cdLocal();
                    break;
            case 5: printf("get\n");
                    pathAux2 = findPath(command, 3);
                    if ( pathAux2 == NULL )
                        continue;
                    rcvGet(m, pathAux2);
                    printf("%s\n", pathAux2);
                    break;
            case 6: printf("put\n");
                    path = findPath(command, 3);
                    size = strlen(path) + 1;
                    m = createMsg(seq, MSG_C, path, size);
                    if ( path == NULL )
                        continue;
                    printf("%s\n", path);
                    break;
            default: error((char *) "Comando inválido", false);
                     break;

        }
    }

    return 0;
}
