#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include "FileManipulate.h"
#include "FDManager.h"
#include <iostream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

#define h_addr h_addr_list[0]
#define tCount transData.tCount

int socketInit(const char* ip,unsigned short port);
void run_client();
void clientLogin();
void updateDiffFiles();
void retriveListHeader();
void processSync();
void processUpload(const string& filename);
void showBar(int progress);

int fd_cmd;
const char* name;
const char* pHostname;
const char* pPort;
vector<fileMeta> localFiles;
vector<fileMeta> diffFiles;

int main(int argc,char** argv){
    if(argc<4)    exit(1);
    pHostname = argv[1];
    pPort = argv[2];
    fd_cmd = socketInit(pHostname,atoi(pPort));   
    name = argv[3];
    run_client();
    close(fd_cmd);
}
int socketInit(const char* hostname,unsigned short port){
    struct hostent* host;
    host = gethostbyname(hostname);
    if(!host)   exit(1);
    sockaddr_in addr_self;
    int fd_self;
    addr_self.sin_family = PF_INET;
    addr_self.sin_port = htons(port);
    memcpy(&addr_self.sin_addr,host->h_addr,host->h_length);
    fd_self = socket(AF_INET,SOCK_STREAM,0);
    if(fd_self < 0)    exit(1);
    if(connect(fd_self,(sockaddr*)&addr_self,sizeof(addr_self))<0)    exit(1);
    return fd_self;
}


void run_client(){
    clientLogin();
    fd_set allset,rset;
    FD_ZERO(&allset);
    FD_SET(fd_cmd,&allset);
    FD_SET(0,&allset);
    int maxfdp1 = max(fd_cmd,0) + 1;
    while(true){
        rset = allset;
        select(maxfdp1,&rset,NULL,NULL,NULL);
        if(FD_ISSET(0,&rset)){
            string cmd;
            cin >> cmd;
            if(cmd=="exit"){
                return;
            }
            else if(cmd=="put"){
                string filename;
                cin >> filename;
                processUpload(filename);
            }
            else if(cmd=="sleep"){
                string timeStr;
                cin >> timeStr;
                int time = stoi(timeStr);
                cout << "Pid: " << (long)getpid() << "  Client starts to sleep" << endl;
                for(int i=1;i<=time;i++){
                    cout << "Pid: " <<  (long)getpid() << "  Sleep " << i << endl;
                    sleep(1);
                }
                cout << "Pid: " <<  (long)getpid() << "  Client wakes up" << endl;
            }
        }
        if(FD_ISSET(fd_cmd,&rset)){
            cmdHeader header;
            bool result = readObject(fd_cmd,header);
            if(result && header.cmdType == 2){
                retriveListHeader();
                processSync();
            }
            else{
                return;
            }
        }
    }

}

void clientLogin(){
    cmdHeader header;
    header.cmdType = 1;
    header.setName(name);
    sendObject(fd_cmd,header);
    readObject(fd_cmd,&header,sizeof(header));
    if(header.cmdType == 2){
        cout <<  "Pid: " << (long)getpid() ;
        printf("  Welcome to the dropbox-like server! : %s\n",name);
        retriveListHeader();
        processSync();
    }
}

void retriveListHeader(){
    transferHeader tH;
    readObject(fd_cmd,&tH,sizeof(tH));
    fileMeta meta;
    diffFiles.clear();
    for(int i=0;i<tH.tCount;i++){
        readObject(fd_cmd,&meta,sizeof(meta));
        bool find = false;
        for(auto& local : localFiles){
           if(local == meta){
               find = true;
               break;
           }
        }
        if(!find){
            diffFiles.push_back(meta);    
        }
    }
}

void processSync(){
    for(auto& entry : diffFiles){
        int fd_data = socketInit(pHostname,atoi(pPort));   
        cmdHeader cH;
        cH.setName(name);
        cH.cmdType = 4;
        cH.meta = entry;
        sendObject(fd_data,cH);
        char* buf = new char[entry.size];
        bzero(buf,entry.size);       
        char* ptr = buf;
        size_t each_size = entry.size / 19;
        size_t partSizes[20];
        for(int i=0;i<19;i++){
            partSizes[i] = each_size * (1+i);
        }
        partSizes[19] = entry.size;
        cout <<  "Pid: " << (long)getpid() << "  [Download] " << entry.name << " Start!\n";      
        int showIndex = 0;
        size_t read_byte = 0;
        size_t remain = entry.size;
        while(read_byte < entry.size){
            size_t read_n;
            read_n = read(fd_data,ptr,remain);
            remain -= read_n;
            ptr += read_n;
            read_byte += read_n;
            for(int i=showIndex;i<20;i++){
                if(read_byte > partSizes[i]){
                    showBar(i);
                    usleep(1000*0);
                }
                else{
                    showIndex = i;
                    break;
                }
            }              
        }
        for(int i=showIndex;i<20;i++)    showBar(i);
        ofstream fout(entry.name);
        fout.write(buf,entry.size);
        cout << endl << "Pid: " << (long)getpid() << "  [Download] " << entry.name << " Finish!\n";
        delete[] buf;
        close(fd_data);
        localFiles.push_back(entry);
    }
}

void processUpload(const string& filename){
    fileBuild file;
    file.load(filename);
    if(file.size > 1200*1024*1024)    return;
    fileMeta& meta = file.meta;
    int fd_data = socketInit(pHostname,atoi(pPort));   
    cmdHeader cH;
    cH.setName(name);
    cH.cmdType = 3;
    cH.meta = file.meta;
    sendObject(fd_data,cH);
    char* ptr = file.data;
    size_t each_size = meta.size / 19;
    size_t partSizes[20];
    for(int i=0;i<19;i++)    partSizes[i] = each_size;
    partSizes[19] = meta.size - 19*each_size;
    cout << "Pid: " << (long)getpid() << "  [Upload] " << meta.name << " Start!\n";
    for(int i=0;i<20;i++){
        int writeSZ = partSizes[i];
        showBar(i);
        usleep(1000*0);
        if(writeSZ > 0){
            write(fd_data,ptr,writeSZ);
            ptr += writeSZ;
        }
    }
    cout << endl << "Pid: " << (long)getpid() << "  [Upload] " << meta.name << " Finish!\n";
    close(fd_data);
    localFiles.push_back(meta);

}

void showBar(int progress){
    cout << "\rPid: " << (long)getpid() << "  Progress : [";
    for(int i=0;i<=progress;i++)    cout << '#';        
    for(int i=progress+1;i<20;i++)    cout << " ";
    cout << "]";
    cout.flush();
}
