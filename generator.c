#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "helper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

void CreateFile(int mbSize) {
    char fileBuffer[1024];
    for(int i =0; i< 1023;i++) fileBuffer[i] = 'T';
    fileBuffer[1023] = '\n';

    char filePath[1000];
    sprintf(filePath, "bigFiles/%dMBfile.txt", mbSize);

    int file = open(filePath, O_WRONLY | O_TRUNC | O_CREAT, mode);
    for(int i=0;i < mbSize * 1024;i++) {
        write(file, fileBuffer, 1024);
    }
    close(file);
}


int main() {
    if(!DirectoryExists("bigFiles")) {
        mkdir("bigFiles", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    CreateFile(10);
    CreateFile(50);
    CreateFile(1024);
}