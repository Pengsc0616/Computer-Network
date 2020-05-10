#include "arpa/inet.h"
#include "unistd.h"
#include "cstdlib"
#include "cstring"
#include <cstdio>
#include <sys/select.h>
#include <sys/time.h>
#include "netdb.h"
#include "signal.h"
#include "errno.h"
#include "fstream"
#include <iostream>
using namespace std;

struct HEADER{
    long number;
    long packet_size;
    long start_word_this_time;
    long start_word_next_time;
    long total_buf_number_this_time;
    int end_of_whole_file;
};
struct window_structure{
    char buf[2048];
    HEADER* ptrH;
}send_packet[1000];

int check[1000];
char buf[2048],sendbuf[2048];
int CheckAll(int maxCheck = 1000){
    for(int i=0;i<maxCheck;i++){
        if(!check[i])    return 0;
    }
    return 1;
}
int main(int argc,char** argv){
    if(argc < 2){
      cout << "Usage : receiver [filename] [port]" << endl;
      exit(1);
    }
    sockaddr_in cliaddr,servaddr;
    socklen_t serv_length;
    int clifd,servfd;
    HEADER header;

    char* target_file = argv[1];
    char* client_port = argv[2];

    clifd = socket(AF_INET,SOCK_DGRAM,0);

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliaddr.sin_port = htons(atoi(client_port));
    bind(clifd,(sockaddr*)&cliaddr,sizeof(cliaddr));
    ofstream file(target_file,ios_base::out | ios_base::binary);
    long cur_start_word_this_time = 0;
    while(1){
        serv_length = sizeof(servaddr);
        int n = recvfrom(clifd,buf,2047,0,(sockaddr*)&servaddr,&serv_length);
        memcpy(&header,buf,sizeof(HEADER));
        if(header.start_word_this_time == cur_start_word_this_time){
            memcpy(send_packet[header.number].buf,buf,n);
            send_packet[header.number].ptrH = (HEADER*)send_packet[header.number].buf;
            check[header.number] = true;
            if(CheckAll(header.total_buf_number_this_time)){
                cur_start_word_this_time = header.start_word_next_time;
                for(int j=0; j<1000; j++)    check[j] = false;
                for(int i=0;i<header.total_buf_number_this_time;i++){
                    file.write(send_packet[i].buf+sizeof(HEADER),send_packet[i].ptrH->packet_size);
                }
                if(send_packet[0].ptrH->end_of_whole_file){
                    memcpy(sendbuf,&header,sizeof(HEADER));
                    sendto(clifd,sendbuf,sizeof(HEADER),0,(sockaddr*)&servaddr,sizeof(servaddr));
                    break;
                }
            }
        }
        memcpy(sendbuf,&header,sizeof(HEADER));
        sendto(clifd,sendbuf,sizeof(HEADER),0,(sockaddr*)&servaddr,sizeof(servaddr));
    }
    close(clifd);
    return 0;
}
