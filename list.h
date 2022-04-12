#include <stdbool.h>

typedef struct File_t {
    char* path;
    bool isDirectory;
    long timestamp;
} File;

typedef struct Node_t {
    File* value;
    struct Node_t* next;
} Node;

typedef struct List_t {
    Node* head;
    int length;
} List;

void Init(List* list);

void Add(List* list, File* value);

File* At(List* list, int index);

void RemoveAt(List* list, int index);

int IndexOf(List* list, char* value);

void Dispose(List* list);

bool UpdateDirectory(char* originDirectory, char* destinationDirectory, bool withDirectories);
