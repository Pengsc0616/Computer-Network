#ifndef INCLUDE_FILE_BUILD_H
#define INCLUDE_FILE_BUILD_H
#include "general.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#define INIT_FILE_SIZE 1024
using std::string;
using std::ifstream;
using std::ofstream;
using std::memcpy;
using std::cout;
using std::cerr;
using std::endl;

struct fileBuild{
    fileBuild();
    ~fileBuild();
    void load(const string& v_filename);
    static void writeAsFile(const string& filename,const void* buf,size_t size);
    void storeBlock(const char* block,size_t bufSize);
    void expand();
    bool loaded;
    char *data;
    size_t size;
    size_t capacity;
    fileMeta meta;
    string filename;
};
void fileBuild::writeAsFile(const string& filename,const void* buf,size_t size){
   ofstream file(filename);
   file.write((const char*)buf,size);
}

fileBuild::fileBuild(){
    loaded = false;
    size = 0;
    capacity = INIT_FILE_SIZE;
    data = new char[capacity];
}

fileBuild::~fileBuild(){
    delete[] data;
}


void fileBuild::load(const string& v_filename){
    loaded = true;
    filename = v_filename;
    char buf[70000];
    ifstream file(v_filename);
    size_t len;
    do{
        file.read(buf,70000);
        len = file.gcount();
        storeBlock(buf,len);
        
    }while(file);
    strncpy(meta.name,filename.c_str(),255);
    meta.name[255-1] = '\0';
    meta.size = size;
}

void fileBuild::storeBlock(const char* buf,size_t bufSize){
    while(size + bufSize > capacity){
        expand();
    }
    memcpy(data+size,buf,bufSize);
    size += bufSize;
}

void fileBuild::expand(){
    char* old = data;
    capacity = capacity * 2;
    data = new char[capacity];
    memcpy(data,old,sizeof(char)*size);
    delete[] old;
}
#endif
