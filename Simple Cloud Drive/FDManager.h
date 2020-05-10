#ifndef INCLUDE_FD_MANAGER_H
#define INCLUDE_FD_MANAGER_H

#include "general.h"
#include <vector>
#include <array>
#include "string.h"
#include "FileManipulate.h"
#include "stdio.h"

using std::vector;
using std::array;
using std::max;

struct fdEntry{
    int fd;
    bool used;
    int type;
    int phase;
    char name[255];
    char* buf;
    char *pBase,*pFront,*pEnd;
    char* rbuf;
    char *rBase,*rFront,*rEnd;
    size_t size;
    bool writeEnd;
    fileMeta meta;
    size_t total;
    bool readEnd;
    string filename;
    fdEntry();
    fdEntry(int fd,int type);
    fdEntry(const fdEntry& rhs) = delete;
    fdEntry& operator=(const fdEntry& rhs) = delete;
    ~fdEntry();
    void clear();
    void remove();
    void setData(const void*const vBuf,size_t vSize);
    void setReadHeader();
    bool flushWrite();
    bool patchRead(const void* buf,size_t vSize);
    void setReadFile(const fileMeta& meta,const string& fullPath);
    void writeBufferAsFile();
};

fdEntry::fdEntry()
:fd(-1),used(false),type(0){
    clear();    
}

fdEntry::fdEntry(int fd,int type)
:fd(fd),used(true),type(type)
{
    clear();   
}


fdEntry::~fdEntry(){
    delete[] buf;
    delete[] rbuf;
}

void fdEntry::clear(){
    buf = new char[70000];
    rbuf = new char[70000];
    pEnd = pFront = pBase = buf;
    rEnd = rFront = rBase = rbuf;
    phase = 0;
    type = 0;
    writeEnd = false;
    readEnd = false;
    filename = "";
}

void fdEntry::remove(){
    close(fd);
    fd = -1;
    used = false;
    clear();
}

void fdEntry::setData(const void* const vBuf ,size_t vSize){
    char* oldbuf = buf;
    size_t old_size = pEnd - pBase;
    buf = new char[old_size + vSize];
    memcpy(buf,pBase,old_size);
    memcpy(buf+old_size,vBuf,vSize);
    pBase = pFront = buf;
    pEnd = pBase + vSize + old_size;
    writeEnd = false;
    total = 0;
    delete[] oldbuf;
}

bool fdEntry::flushWrite(){
    if(pBase < pEnd){
        size_t nW = write(fd,pBase,pEnd - pBase);
        if(nW < 0){
            if(errno != EWOULDBLOCK){
                exit(1);
            }
        }
        total += nW;
        pBase += nW;
    }
    else{
        if(!writeEnd){
        }
        if (total > 0){
            total = 0;
        }
        writeEnd = true;
    }
    return !writeEnd;
}



void fdEntry::setReadFile(const fileMeta& meta,const string& fullPath){
    delete[] rbuf;
    rbuf = new char[meta.size];
    rBase = rbuf;
    rEnd = rbuf + meta.size;
    filename = fullPath;
}

void fdEntry::setReadHeader(){
    delete[] rbuf;
    rbuf = new char[sizeof(cmdHeader)];
    rBase = rbuf;
    rEnd = rbuf + sizeof(cmdHeader);
}

void fdEntry::writeBufferAsFile(){
    fileBuild::writeAsFile(filename,rbuf,rEnd - rbuf);
}

bool fdEntry::patchRead(const void* vBuf,size_t vSize){
    memcpy(rBase,vBuf,vSize);
    rBase += vSize;
    if(rBase >= rEnd){
        return false;
    }
    else{
        return true;
    }
}

struct FDManager{
    FDManager();
    fdEntry& addFD(int fd,int type); 
    int maxfdp1;
    array<fdEntry,100> data;
    int size;
};

FDManager::FDManager(){
    maxfdp1 = 0;
    size = 0;
}

fdEntry& FDManager::addFD(int fd,int type = 0){
    fdEntry* pE;
    bool found = false;
    for(int i=0;i<size;i++){
        auto& entry = data[i];
        if(!entry.used){
            pE = &entry;
            entry.fd = fd;
            entry.used = true;
            found = true;
            break;
        }     
    }
    if(!found){
        data[size].fd = fd;
        data[size].type = type;
        data[size].used = true;
        pE = &data[size];
        ++size;
    }
    maxfdp1 = max(maxfdp1,fd+1);
    return *pE;
}
#endif
