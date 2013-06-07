#include <cstdio>
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
#include <cstring>
#include <string>

using namespace std;

// função que aborta a execução caso haja algum erro
void error(char *erro, bool exit_){
    printf("error: %s\n", erro);

    if ( exit_ )
        exit(1);
}

// função para criar um raw socket
int createSocket(char *device){
    int    sd, deviceid;
    struct ifreq ifr;
    struct sockaddr_ll sll;
    struct packet_mreq mr;

    sd = socket(PF_PACKET, SOCK_RAW, 0);
    if ( sd == -1 )
        error((char *) "failed opening socket", true);

    // get device ID
    memset(&ifr, 0, sizeof(struct ifreq));
    memcpy(ifr.ifr_name, device, strlen(device));
    if ( ioctl(sd, SIOCGIFINDEX, &ifr) == -1 )
        error((char *) "failed getting device", true);

    deviceid = ifr.ifr_ifindex;

    memset(&sll, 0, sizeof(sll));
    sll.sll_family   = AF_PACKET;
    sll.sll_ifindex  = deviceid;
    sll.sll_protocol = htons(ETH_P_ALL);
    if ( bind(sd, (struct sockaddr *)&sll, sizeof(sll)) == -1 )
      error((char *) "failed calling bind", true);

    // modo promiscuo
    memset(&mr, 0, sizeof(mr));

    mr.mr_ifindex = deviceid;
    mr.mr_type    = PACKET_MR_PROMISC;

    if ( setsockopt(sd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&mr,sizeof(mr) ) == -1 )
      error((char *) "failed calling setsockopt", true);

    return sd;
}
