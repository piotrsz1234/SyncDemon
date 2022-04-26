#include "helper.h"
#include <stdbool.h>
#include <time.h>

int main() {
    
    UpdateDirectory("/mnt/c/Users/HP/Desktop/FileSync Test/origin", "/mnt/c/Users/HP/Desktop/FileSync Test/destination", true, 1, 0);
    long timestamp = time(0);
    sleep(1);
    UpdateDirectory("/mnt/c/Users/HP/Desktop/FileSync Test/origin", "/mnt/c/Users/HP/Desktop/FileSync Test/destination", true, 1, timestamp);
    return 0;
}