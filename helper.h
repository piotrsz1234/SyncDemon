
#include <stdbool.h>
#include "list.h"

void ReportError(int errNo);

void ReportTrace(char* text);

bool DeleteFile(char* path);

bool DeleteDirectory(char* path);

char* CombinePaths(char* p1, char* p2);

bool ReadWriteCopyFile(char* originPath, char* fileName, char* destinationPath);

List* GetFilesFromDirectory(char* directoryPath);