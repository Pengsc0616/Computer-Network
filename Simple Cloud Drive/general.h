#ifndef INCLUDE_GENERAL_H
#define INCLUDE_GENERAL_H
#include "stdio.h"
#define WRITE_TO_LOG

struct fileMeta{
    char name[255];
    size_t size;
    bool operator==(const fileMeta& rhs);
};

bool fileMeta::operator==(const fileMeta& rhs){
    return strcmp(name,rhs.name)==0;
}


struct cmdHeader{
    char name[255];
    int cmdType;
    fileMeta meta;
    void setName(const char* vName);
};

void cmdHeader::setName(const char* vName){
    strncpy(name,vName,255);
    name[255-1] = '\0';
}


struct transferHeader{
    int type;
    union {
        size_t tCount;
        fileMeta tMeta;
    } transData;
};
#define tCount transData.tCount
#define tMeta transData.tMeta


void setNonBlock(int fd){
    int flag=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
}

void sendObject(int fd,void* obj,size_t size){
    write(fd,obj,size);
}

template<typename T>
void sendObject(int fd,T& obj){
    write(fd,&obj,sizeof(obj));
}

void readObject(int fd,void* Pobj,size_t size){
    char* obj = (char*)Pobj;
    char buf[70000];
    size_t offset = 0;
    int read_n;
    while(size > 0){
        read_n = read(fd,buf,size > 70000 ? 70000 : size );
        if(read_n <= 0){
            return;
        }
        size -= read_n;
        memcpy(obj+offset,buf,read_n);
        offset += read_n;
    }
}

template<typename T>
bool readObject(int fd,T& rObj){
    char* obj = (char*)&rObj;
    size_t size =  sizeof(rObj); 
    char buf[70000];
    int read_n;
    size_t offset = 0;
    while(size > 0){
        read_n = read(fd,buf,size > 70000 ? 70000 : size );
        if(read_n <= 0){
            return false;
        }
        size -= read_n;
        memcpy(obj+offset,buf,read_n);
        offset += read_n;
    }
    return true;
}

#endif
