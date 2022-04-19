#include "list.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void Init(List* list) {
    list->head = NULL;
    list->length = 0;
}

void Add(List *list, File *value)
{
    if (list->head == NULL)
    {
        list->head = malloc(sizeof(Node));
        list->head->value = value;
        list->head->next = NULL;
    }
    else
    {
        Node *temp = list->head;
        while (temp->next == NULL)
        {
            temp = temp->next;
        }

        temp->next = malloc(sizeof(Node));
        temp->next->value = value;
        temp->next->next = NULL;
    }
    list->length++;
}

File *At(List *list, int index)
{
    Node *temp = list->head;
    for (int i = 0; i < index; i++)
    {
        temp = temp->next;
    }

    return temp->value;
}

void RemoveAt(List *list, int index)
{
    Node *temp = list->head;
    for (int i = 0; i < index; i++)
    {
        temp = temp->next;
    }
    Node *toFree = temp->next;

    temp->next = temp->next->next;
    list->length--;
    free(toFree->value);
    free(toFree);
}

int IndexOf(List *list, char* path)
{
    Node *temp = list->head;
    for (int i = 0; i < list->length; i++)
    {
        if (strcmp(temp->value->path, path) == 0) {
            return i;
        }
    }

    return -1;
}

void Dispose(List* list) {
    Node *temp = list->head;
    Node* temp2 = NULL;
    for(int i = 0; i < list->length; i++) {
        temp2 = temp;
        temp = temp->next;
        free(temp2->value->path);
        free(temp2->value);
        free(temp2);
    }
}