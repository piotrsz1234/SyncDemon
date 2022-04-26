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
#include <sys/io.h>
#include <sys/mman.h>
#include <syslog.h>

#include "helper.h"
#include "list.h"

#define FILE_TYPE 1
#define DIRECTORY_TYPE 2

void GetErrorMessage(char* message, int errNo) {
	sprintf(message, "SYNC DEMON: %s", strerror(errNo));
}

void ReportError(int errNo)
{
	char message[10000];
	GetErrorMessage(message, errNo);
	syslog(LOG_ERR, "%s", message);
	closelog();
}

void ReportTrace(char *text)
{
	syslog(LOG_NOTICE, "SYNC DEMON: %s", text);
	closelog();
}

bool DeleteFile(char *path)
{
	if (unlink(path) == -1)
	{
		ReportError(errno);
		return false;
	}
	return true;
}

int GetFileType(char *path)
{
	struct stat st;
	lstat(path, &st);
	if (S_ISDIR(st.st_mode))
	{
		return DIRECTORY_TYPE;
	}
	if (S_ISREG(st.st_mode))
	{
		return FILE_TYPE;
	}

	return -1;
}

long GetTimestamp(char *path)
{
	struct stat st;
	lstat(path, &st);
	return st.st_mtime;
}

bool DeleteDirectory(char *path)
{
	List *files = GetFilesFromDirectory(path);
	bool result = true;
	for (int i = 0; i < files->length; i++)
	{
		File* current = At(files, i);
		char* currentPath = CombinePaths(path, current->path);

		if(current->isDirectory == true) {
			result &= DeleteDirectory(currentPath);
		} else {
			result &= DeleteFile(currentPath);
		}
		
		if(result == false) {
			break;
		}
	}

	if (result && rmdir(path) == -1)
	{
		ReportError(errno);
		result = false;
	}

	Dispose(files);
	return result;
}

char *CombinePaths(char *p1, char *p2)
{
	char *path = malloc(sizeof(char) * (PATH_MAX - 1));
	size_t p1Length = strlen(p1);
	if (p1[p1Length - 1] != '/' && p2[0] != '/')
	{
		sprintf(path, "%s/%s", p1, p2);
	}
	else
	{
		sprintf(path, "%s%s", p1, p2);
	}

	return path;
}

bool ReadWriteCopyFile(char *originPath, char *fileName, char *destinationPath)
{
	char *buffor = malloc(sizeof(char) * 1024 * 1024);
	int bufforSize = 1024 * 1024;
	char *originFilePath = CombinePaths(originPath, fileName);
	char *destinationFilePath = CombinePaths(destinationPath, fileName);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	bool output = true;
	int originalFile = open(originFilePath, O_RDONLY);

	if (originalFile == -1)
	{
		ReportError(errno);
		output = false;
	}
	else
	{
		int destinationFile = open(destinationFilePath, O_WRONLY | O_TRUNC | O_CREAT, mode);
		if (destinationFile == -1)
		{
			ReportError(errno);
			output = false;
		}
		else
		{
			size_t readBytesCount = 0;
			do
			{
				readBytesCount = read(originalFile, buffor, bufforSize);
				if (readBytesCount == -1)
				{
					ReportError(errno);
					output = false;
					break;
				}

				int result = write(destinationFile, buffor, readBytesCount);

				if (result == -1)
				{
					ReportError(errno);
					output = false;
					break;
				}

			} while (readBytesCount == bufforSize);
		}
	}

	free(buffor);
	free(originFilePath);
	free(destinationFilePath);
	return output;
}

bool CreateAndSyncDirectory(char *originPath, char *destinationPath, char *directoryName, bool withDirectories, int minSizeForMMap, long lastTimestamp)
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	char *originDirectoryPath = CombinePaths(originPath, directoryName);
	char *destinationDirectoryPath = CombinePaths(destinationPath, directoryName);
	bool result = true;
	if (mkdir(destinationDirectoryPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
	{
		result = false;
	}
	else
	{
		result &= UpdateDirectory(originDirectoryPath, destinationDirectoryPath, withDirectories, minSizeForMMap, lastTimestamp);
	}
	free(originDirectoryPath);
	free(destinationDirectoryPath);
	return result;
}

bool UpdateFile(char *originPath, char *fileName, char *destinationPath, int minSizeForMMap)
{
	char *originFilePath = CombinePaths(originPath, fileName);
	char *desitnationFilePath = CombinePaths(destinationPath, fileName);
	char* message = malloc(sizeof(char) * 10000);
	sprintf(message, "Making copy of file: %s", originFilePath);
	ReportTrace(message);
	free(message);
	bool result = DeleteFile(desitnationFilePath);
	if(result == true && GetFileSize(originFilePath) >= minSizeForMMap * 1024 * 1024) {
		result &= MMapWriteCopyFile(originPath, fileName, destinationPath);
	} else if(result == true) {
		result &= ReadWriteCopyFile(originPath, fileName, destinationPath);
	}
	free(originFilePath);
	free(desitnationFilePath);
	return result;
}

bool UpdateDirectory(char *originDirectory, char *destinationDirectory, bool withDirectories, int minSizeForMMap, long lastTimestamp)
{
	List *originFiles = GetFilesFromDirectory(originDirectory);
	List *destinationFiles = GetFilesFromDirectory(destinationDirectory);
	bool result = true;
	 for (int i = 0; i < originFiles->length; i++)
	{
		File *current = At(originFiles, i);
		int index = IndexOf(destinationFiles, current->path);
		if (index >= 0)
		{
			if (current->isDirectory && withDirectories)
			{
				result &= UpdateDirectory(CombinePaths(originDirectory, current->path), CombinePaths(destinationDirectory, current->path), withDirectories, minSizeForMMap, lastTimestamp);
			}
			else if (current->isDirectory == false)
			{
				long destinationTimeStamp = At(destinationFiles, index)->timestamp;
				if (destinationTimeStamp > current->timestamp && destinationTimeStamp > lastTimestamp)
				{
					result &= UpdateFile(originDirectory, current->path, destinationDirectory, minSizeForMMap);
				}
			}
		}
		else
		{
			if (current->isDirectory == true && withDirectories)
			{
				result &= CreateAndSyncDirectory(originDirectory, destinationDirectory, current->path, withDirectories, minSizeForMMap, lastTimestamp);
			}
			else if (current->isDirectory == false)
			{
				char* originFilePath = CombinePaths(originDirectory, current->path);
				if(GetFileSize(originFilePath) >= minSizeForMMap * 1024 * 1024) {
					result &= MMapWriteCopyFile(originDirectory, current->path, destinationDirectory);
				} else {
					result &= ReadWriteCopyFile(originDirectory, current->path, destinationDirectory);
				}
				free(originFilePath);
			}
		}
	}
	char* message = malloc(sizeof(char) * 1000);
	for (int i = 0; i < destinationFiles->length; i++)
	{
		File *current = At(destinationFiles, i);
		
		if (IndexOf(originFiles, current->path) < 0)
		{
			char *path = CombinePaths(destinationDirectory, current->path);
			if (current->isDirectory)
			{
				sprintf(message, "Removing directory: %s", path);
				result &= DeleteDirectory(path);
			}
			else
			{
				sprintf(message, "Removing file: %s", path);
				result &= DeleteFile(path);
			}
			ReportTrace(message);
			free(path);
		}
	}
	free(message);
	Dispose(originFiles);
	Dispose(destinationFiles);

	return result;
}

List *GetFilesFromDirectory(char *directoryPath)
{
	List *output = malloc(sizeof(List));
	Init(output);
	DIR *dir = opendir(directoryPath);
	if (dir == NULL)
	{
		Dispose(output);
		return false;
	}
	int path_len = strlen(directoryPath);
	struct dirent *entry;
	while (((entry = readdir(dir)) != NULL))
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}
		char *path = CombinePaths(directoryPath, entry->d_name);
		int type = GetFileType(path);
		File *temp = malloc(sizeof(File));
		char* copyOfFileName = malloc(sizeof(char) * (strlen(entry->d_name) + 5));
		sprintf(copyOfFileName, "%s", entry->d_name);
		temp->path = copyOfFileName;
		if (type == DIRECTORY_TYPE)
		{
			temp->isDirectory = true;
			temp->timestamp = -1;
		}
		else if (type == FILE_TYPE)
		{
			temp->isDirectory = false;
			temp->timestamp = GetTimestamp(path);
		}
		else
		{
			free(temp);
			continue;
		}
		Add(output, temp);
	}
	closedir(dir);
	return output;
}

long int GetFileSize(char* filePath) {
	struct stat st;
	lstat(filePath, &st);
	return st.st_size;
}

bool MMapWriteCopyFile(char* originPath, char* fileName, char* destinationPath) {
	char *originFilePath = CombinePaths(originPath, fileName);
	char *destinationFilePath = CombinePaths(destinationPath, fileName);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
	bool output = true;

	int originalFile = open(originFilePath, O_RDONLY);
	if (originalFile == -1)
	{
		ReportError(errno);
		output = false;
	}
	else
	{
		
		long int fileSize = GetFileSize(originFilePath);

		if (fileSize == 0)
		{
			ReportError(errno);
		}
		
		void * region = mmap(NULL, fileSize,
			PROT_READ | PROT_WRITE, MAP_PRIVATE,
			originalFile, 0);

		if(((long) region) == -1) {
			ReportError(errno);
		} else {
			int destinationFile = open(destinationFilePath, O_WRONLY | O_TRUNC | O_CREAT, mode);
			if (destinationFile == -1)
			{
				ReportError(errno);
				output = false;
			} else {
				int result = write(destinationFile, region, fileSize);
				if (result == -1)
				{
					close(destinationFile);
					DeleteFile(destinationFilePath);
					ReportError(errno);
					output = false;
				}
				close(destinationFile);
			}
		}

		close(originalFile);
		munmap(region, fileSize);
	}

	free(originFilePath);
	free(destinationFilePath);
	return output;
}