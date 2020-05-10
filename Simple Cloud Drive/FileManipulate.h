#ifndef INCLUDE_FILE_MANIPULATE_H
#define INCLUDE_FILE_MANIPULATE_H
#include "FileBuild.h"
#include "vector"
#include <iostream>
#include <fstream>
#include <sstream>
#include "cstring"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sys/stat.h"


#define DIR_MODE (S_IRWXU | S_IRGRP | S_IXGRP)

using std::stringstream;
using std::ofstream;
using std::vector;
using std::strcmp;
using std::string;

struct userEntry{
    char name[255];
    vector<fileMeta> files;
    string dir_path;
    userEntry(const char* vName);
    void createWelcomeFile(int id = 0);
    transferHeader generateListHeader();
    string filePath(const char* filename);
};

userEntry::userEntry(const char* vName){
    strcpy(name,vName);
    dir_path = name;
    dir_path += "/";
}
string userEntry::filePath(const char* filename){
    return dir_path + filename;
}

void userEntry::createWelcomeFile(int id ){
    char buf[70000];
    bzero(buf,70000);
    snprintf(buf,70000,"Welcome Seq_%d",id);
    string filename(buf);
    ofstream fout(dir_path + filename);
    string content("Welcome Message for ");
    for(int i=0;i<id*1000;i++){
        content += name;    
    }
    fileMeta meta;
    strcpy(meta.name,filename.c_str());
    size_t fileSize = 0;
    size_t nWrite;
    fout.write(content.c_str(),content.size());
    meta.size = content.size();
    files.push_back(meta);
}

transferHeader userEntry::generateListHeader(){
    transferHeader tH;
    tH.type = 1;
    tH.tCount = files.size();
    return tH;
}

struct fileManipulate{
    fileManipulate();
    bool checkUserExist(const char* name);
    void addUser(const char* name);
    userEntry& getUserEntry(const char* name);
    vector<userEntry> data; 
};

fileManipulate::fileManipulate(){}

bool fileManipulate::checkUserExist(const char* name){
    for(auto& entry:data){
        if(strcmp(name,entry.name)==0){
            return true;
        }    
    }
    return false;
}

void fileManipulate::addUser(const char* name){
    if(!checkUserExist(name)){
        userEntry ent(name);
        char path[70000];
        snprintf(path,70000,"./%s",name);
        mkdir(path,DIR_MODE);
        data.push_back(std::move(ent));
    }    
}

userEntry& fileManipulate::getUserEntry(const char* name){
    for(auto& entry : data){
        if(strcmp(entry.name,name)==0){
            return entry;
        }
    }
}

#endif
