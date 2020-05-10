
#include "iostream"
#include "string"
#include "cstring"
#include "cstdio"
#include "cstdlib"
#include <vector>

#include "sys/socket.h"
#include "arpa/inet.h"
#include "netinet/in.h"
#include "unistd.h"
#include "netdb.h"
#include "errno.h"
#include "fcntl.h"
#include "cstring"

#include "FileManipulate.h"
#include "FDManager.h"
unsigned short bindPort;

using namespace std;

int socketInit();
void run_server();
void processList(fdEntry& entry);
void processFileDownload(fdEntry& entry,cmdHeader header);
int fd_listen;
FDManager* pFD;
fileManipulate* pFM;

int main(int argc,char** argv){
    if(argc < 2){
        cerr << "Usage : ./server <port>" << endl;
        exit(1);
    }
    bindPort = atoi(argv[1]);   
    fd_listen = socketInit(); 
    run_server();
    close(fd_listen);
    return 0;
}

void run_server(){
    pFD = new FDManager;
    FDManager& FDm = *pFD;
    FDm.addFD(fd_listen,2);

    pFM = new fileManipulate;
    fileManipulate& fileM = *pFM;

    char buf[70000];
    fd_set allRset,allWset;
    fd_set rset,wset;
    FD_ZERO(&allRset);
    FD_ZERO(&allWset);
    FD_SET(fd_listen,&allRset);
    int nready;
    sockaddr_in addr_peer;
    socklen_t len_peer;
    while(true){
        rset = allRset;
        wset = allWset;
        nready = select(FDm.maxfdp1,&rset,&wset,NULL,NULL);
        if(nready > 0){
            int chk_size = FDm.size;
            for(int i=0;i<chk_size;i++){
                auto& entry = FDm.data[i];
                if(!entry.used){
                    continue;
                }
                if(FD_ISSET(entry.fd,&rset)){
                    if(entry.type == 2){
                        len_peer = sizeof(sockaddr_in);
                        int new_fd = accept(fd_listen,(sockaddr*)&addr_peer,&len_peer);
                        auto& newEntry = FDm.addFD(new_fd,0);
                        FD_SET(new_fd,&allRset);
                        FD_SET(new_fd,&allWset);
                        setNonBlock(new_fd);
                        newEntry.setReadHeader();
                    }
                    else{
                        int read_n,readSize;
                        if(entry.phase == 0){
                            readSize = entry.rEnd - entry.rBase;
                        }
                        else{
                            readSize = 70000;
                        }
                        read_n = read(entry.fd,&buf,readSize);
                        if(read_n <= 0){
                            if(errno != EWOULDBLOCK){
                                close(entry.fd);
                                FD_CLR(entry.fd,&allRset);
                                FD_CLR(entry.fd,&allWset);
                                entry.remove();
                            }
                        }
                        else{
                            if(entry.phase == 0 ){
                                if(!entry.patchRead(buf,read_n)){
                                    FD_SET(entry.fd,&allWset);
                                    cmdHeader& main_h = *(cmdHeader*)(entry.rbuf);
                                    switch(main_h.cmdType){
                                    case 1:
                                        fileM.addUser(main_h.name);
                                        strcpy(entry.name,main_h.name);
                                        entry.type = 1;
                                        processList(entry);
                                        entry.setReadHeader();
                                        break;
                                    case 4:
                                        strcpy(entry.name,main_h.name);
                                        entry.phase = 1;
                                        entry.type = 3;
                                        processFileDownload(entry,main_h);
                                        break;
                                    case 3:
                                        strcpy(entry.name,main_h.name);
                                        entry.type = 4;
                                        entry.phase = 1;
                                        auto& user = pFM->getUserEntry(entry.name);
                                        entry.setReadFile(main_h.meta,user.filePath(main_h.meta.name));
                                        entry.meta = main_h.meta;
                                        FD_CLR(entry.fd,&allWset);
                                        FD_CLR(entry.fd,&wset);
                                        break;
                                    }
                                }  
                            }
                            else if(entry.phase == 1 && entry.type == 4){
                                if(!entry.patchRead(buf,read_n)){
                                    entry.writeBufferAsFile();
                                    auto& user = fileM.getUserEntry(entry.name);
                                    user.files.push_back(entry.meta);
                                    for(int k=0;k<FDm.size;k++){
                                        auto& other_entry = FDm.data[k];
                                        if(other_entry.used && other_entry.type == 1 && strcmp(other_entry.name,entry.name)==0){
                                            processList(other_entry);
                                        }
                                    }
                                    FD_CLR(entry.fd,&allRset);
                                    entry.remove();
                                }
                            }
                        }
                    }
                }
                if(FD_ISSET(entry.fd,&wset)){ 
                    if(entry.type != 2){
                        if(!entry.flushWrite() && entry.type == 3){
                            FD_CLR(entry.fd,&allWset); 
                            FD_CLR(entry.fd,&allRset); 
                            entry.remove();
                        }
                    }
                }               
            }
        }
    }
    delete pFM;
    delete pFD;
}

int socketInit(){
    sockaddr_in addr_self;
    int fd_self;

    addr_self.sin_family = PF_INET;
    addr_self.sin_port = htons(bindPort);
    addr_self.sin_addr.s_addr = INADDR_ANY;

    fd_self = socket(AF_INET,SOCK_STREAM,0);

    if(fd_self < 0)    exit(1);
    if(bind(fd_self,(sockaddr*)&addr_self,sizeof(addr_self))<0)    exit(1);
    if(listen(fd_self,10)<0)    exit(1);
    return fd_self;
}

void processList(fdEntry& entry){
    const char* name = entry.name;
    cmdHeader header;
    header.cmdType = 2;
    header.setName(name);
    entry.setData(&header,sizeof(header)); 
    auto& user = pFM->getUserEntry(name);
    transferHeader tH = user.generateListHeader();
    entry.setData(&tH,sizeof(tH));
    for(auto& FEntry : user.files){
        entry.setData(&FEntry,sizeof(FEntry));
    }
}

void processFileDownload(fdEntry& entry,cmdHeader header){
    auto& user = pFM->getUserEntry(entry.name);
    fileBuild file;
    file.load(user.filePath(header.meta.name));
    entry.setData(file.data,file.size);
} 
