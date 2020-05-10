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
typedef void (*Sigfunc)(int);

void handle_sig_alrm(int signo){
    alarm(1);
    return ;
}

char buf[2048];
int main(int argc,char** argv){
    if(argc < 4){
      cout << "Usage : sender_sigalrm [filename] [IP] [port]" << endl;
      exit(1);
    }
    sockaddr_in servaddr,cliaddr;
    int servfd,clifd;

    char *target_file = argv[1];
    char *client_host = argv[2];
    char *client_port = argv[3];
    hostent* host = gethostbyname(client_host);

    servfd = socket(AF_INET,SOCK_DGRAM,0);

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(atoi(client_port));
    memcpy(&cliaddr.sin_addr,*host->h_addr_list,sizeof(in_addr));
    connect(servfd,(sockaddr*)&cliaddr,sizeof(cliaddr));
   
    Sigfunc old_handle = signal(SIGALRM,handle_sig_alrm);
    siginterrupt(SIGALRM,1);
    alarm(1);

    long start_word_this_time = 0,start_word_next_time;
    int finished = 0;
    int data_count;
    int send_ok = false;
    HEADER header,recv_header;
    ifstream file(target_file,ios_base::in | ios_base::binary);

    const int header_size = sizeof(HEADER);
    long temp_calculate = 0;
    const int frame_size = header_size + 960;
    int send_size;
    int total_buf_number_this_time;
    while(!finished){
        int i;
        for(int j=0; j<1000; j++)    check[j] = false;
        for(i=0;i<1000 &&!finished;i++){    
            file.read(send_packet[i].buf+header_size,960);
            header.number = i;
            header.start_word_this_time = temp_calculate;
            data_count = file.gcount();
            finished = file.eof();
            header.packet_size = data_count;
            start_word_this_time = start_word_this_time + data_count;
            send_packet[i].ptrH = (HEADER*)send_packet[i].buf;
            memcpy(send_packet[i].buf,&header,sizeof(header));
        }
        total_buf_number_this_time = i;
        temp_calculate = start_word_this_time;
        PacketLose:
        send_ok = false;
        for(int i=0;i<total_buf_number_this_time;i++)
        {
            if(check[i])    continue;
            send_packet[i].ptrH->end_of_whole_file = finished;
            send_packet[i].ptrH->start_word_next_time = start_word_this_time;
            send_packet[i].ptrH->total_buf_number_this_time = total_buf_number_this_time;
            send_size = sizeof(HEADER) + send_packet[i].ptrH->packet_size; 
            write(servfd,send_packet[i].buf,send_size);
        }
        while(!send_ok){
            int n = read(servfd,buf,2048);
            if(n < 0){
                if(errno== EINTR)    goto PacketLose;
                else{
                    cout << "Lose connection." << endl;
                    finished = true;
                    break;
                }
            }
            else{
                memcpy(&recv_header,buf,sizeof(recv_header));
                if(recv_header.start_word_this_time == send_packet[0].ptrH->start_word_this_time){ 
                    check[recv_header.number] = true;
                    send_ok = true;
                    for(int k=0; k<total_buf_number_this_time; k++){
                        if(!check[k]){
                            send_ok = false;
                            break;
                        }
                    }
                }
            }   
        }
    }
    close(servfd);
    return 0;
}
