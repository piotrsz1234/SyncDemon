#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include "helper.h"
#include "list.h"

#define FILE_TYPE 1
#define DIRECTORY_TYPE 2

void ReportError(int errNo) {
	
}

void ReportTrace(char* text) {

}

bool DeleteFile(char* path) {
	if(unlink(path) == -1) {
		ReportError(errno);
		return false;
	}
	return true;
}

int GetFileType(char* path) {
	struct stat st;
    lstat (path, &st);
    if (S_ISDIR (st.st_mode)) {
    	return DIRECTORY_TYPE;
    }
    if(S_ISREG (st.st_mode)) {
    	return FILE_TYPE;
    }
    
    return -1;
}

long GetTimestamp(char* path) {
	struct stat st;
    lstat (path, &st);
    return st.st_mtime;
}

bool DeleteDirectory(char* path) {
	DIR* dir = opendir (path);
	char entry_path[PATH_MAX + 1];
	struct dirent* entry;
	size_t path_len = strlen(path);
	while ((entry = readdir (dir)) != NULL) {
        strncpy (entry_path + path_len, entry->d_name,
        sizeof (entry_path) - path_len);
        int type = GetFileType (entry_path);
        if(type == DIRECTORY_TYPE) {
        	DeleteDirectory(entry_path);
        } else if(type == FILE_TYPE) {
        	DeleteFile(entry_path);
        }
    }
    
    if(rmdir(path) == -1) {
    	ReportError(errno);
    	return false;
    }
    return true;
}

char* CombinePaths(char* p1, char* p2) {
	char* path = malloc(sizeof(char) * (PATH_MAX - 1));
	size_t p1Length = strlen(p1);
	if(p1[p1Length - 1] != '\\' && p2[0] != '\\') {
		sprintf(path, "%s\\%s", p1, p2);
		return path;
	}
	sprintf(path, "%s%s", p1, p2);
	return path;
}

bool ReadWriteCopyFile(char* originPath, char* fileName, char* destinationPath) {
	char* buffor = malloc(sizeof(char) * 1024 * 1024);
	int bufforSize = 1024 * 1024;
	char* originFilePath = CombinePaths(originPath, fileName);
	char* destinationFilePath = CombinePaths(destinationPath, fileName);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH; 
	bool output = true;
	
	int originalFile = open (originFilePath, O_RDONLY); 
	
	if(originalFile == -1) {
		ReportError(errno);
		output = false;
	} else {
		int destinationFile = open(destinationFilePath, O_WRONLY | O_TRUNC | O_CREAT, mode);
		if(destinationFile == -1) {
			ReportError(errno);
			output = false;
		} else {
			size_t readBytesCount = 0;
			do {
				readBytesCount = read(originalFile, buffor, sizeof(buffor));
				
				if(readBytesCount == -1) {
					ReportError(errno);
					output = false;
					break;
				}
				
				int result = write(destinationFile, buffor, readBytesCount);
				
				if(result == -1) {
					ReportError(errno);
					output = false;
					break;
				}
				
			} while(readBytesCount == bufforSize);
		}
	}
	
	free(buffor);
	free(originFilePath);
	free(destinationFilePath);
	return output;
}

bool UpdateFile(char* originPath, char* fileName, char* destinationPath) {
	char* originFilePath = CombinePaths(originPath, fileName);
	char* desitnationFilePath = CombinePaths(destinationPath, fileName);
	
	bool result = DeleteFile(desitnationFilePath) && ReadWriteCopyFile(originPath, fileName, destinationPath);

	free(originFilePath);
	free(desitnationFilePath);

	return result;
}

bool UpdateDirectory(char* originDirectory, char* destinationDirectory, bool withDirectories) {
	List* originFiles = GetFilesFromDirectory(originDirectory);
	List* destinationFiles = GetFilesFromDirectory(destinationDirectory);

	bool result = true;

	for(int i = 0; i < originFiles->length; i++) {
		File* current = At(originFiles, i);
		int index = IndexOf(destinationFiles, current->path);
		if(index >= 0) {
			if(current->isDirectory && withDirectories) {
				result &= UpdateDirectory(CombinePaths(originDirectory, current->path), CombinePaths(destinationDirectory, current->path), withDirectories);
			} else if(current->isDirectory == false) {
				if(At(destinationFiles, index)->timestamp < current->timestamp) {
					result &= UpdateFile(originDirectory, current->path, destinationDirectory);
				}
			}
		} else {
			if(current->isDirectory && withDirectories) {
				//createDir
				//Update new directory
			} else if(current->isDirectory == false) {
				result &= ReadWriteCopyFile(originDirectory, current->path, destinationDirectory);
			}
		}
	}
    
	for(int i = 0; i < destinationFiles->length; i++) {
		File* current = At(destinationFiles, i);
		if(IndexOf(originFiles, current->path) < 0) {
			char* path = CombinePaths(destinationDirectory, current->path);
			if(current->isDirectory) {
				result &= DeleteDirectory(path);
			} else {
				result &= DeleteFile(path);
			}
			free(path);
		}
	}

    return result;
}

List* GetFilesFromDirectory(char* directoryPath) {
	List* output = malloc(sizeof(List));
	Init(output);
	printf("test\n");
	printf(directoryPath);
	DIR* dir = opendir (directoryPath);
	int path_len = strlen(directoryPath);
	struct dirent* entry;
	printf("\ntest\n");
	while (dir != NULL && (entry = readdir (dir)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
			continue;
		}

		printf("test\n %d", sizeof(File));
        char* path = CombinePaths(directoryPath, entry->d_name);
        int type = GetFileType (path);
		File* temp = malloc(sizeof(File));
		temp->isDirectory = true;
		temp->path = path;
		
		if(type == DIRECTORY_TYPE) {
			temp->timestamp = -1;
		} else {
			temp->timestamp = GetTimestamp(path);
		}
		printf("test\n");
		Add(output, temp);
		printf("    test2\n");
    }
	printf("Koniec");
	return output;
}