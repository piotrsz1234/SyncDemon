#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "helper.h"
#include "list.h"
#include <time.h>
#include <sys/time.h>

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
    // if(!DirectoryExists("bigFiles")) {
    //     mkdir("bigFiles", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // }
    // for(int i=5;i <= 100;i += 5) {
    //     CreateFile(i * 10);
    // }

    char originPath[1000];
    sprintf(originPath, "/mnt/c/Users/HP/Projects/FileSync/bigFiles");
    char destinationPath[1000];
    sprintf(destinationPath, "/mnt/c/Users/HP/Projects/FileSync/bigFilesCopy");
    long readWriteCopyTime = 0;
    long mmapWriteCopyTime = 0;

    List* files = GetFilesFromDirectory(originPath);

    for(int i=0;i < files->length;i++) {
        File* current = At(files, i);
        char* destinationFilePath = CombinePaths(destinationPath, current->path);
        struct timeval stop, start;
        gettimeofday(&start, NULL);
        ReadWriteCopyFile(originPath, current->path, destinationPath);
        gettimeofday(&stop, NULL);
        readWriteCopyTime =  (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
        DeleteFile(destinationFilePath);
        gettimeofday(&start, NULL);
        MMapWriteCopyFile(originPath, current->path, destinationPath);
         gettimeofday(&stop, NULL);
        mmapWriteCopyTime = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
        free(destinationFilePath);
        printf("%s file readwrite copy time: %ld us mmap copy time: %ld us\n", current->path, readWriteCopyTime, mmapWriteCopyTime);
    }

}