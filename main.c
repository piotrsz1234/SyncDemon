#include "helper.h"
#include <stdbool.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

static bool recursive = false;
static int mmapMinSize = 1;
static int sleepTime = 5;
static char* originPath;
static char* destinationPath;

bool ParseArguments(int argc, char** argv) {
    static struct option long_options[] = {
        {"recursive", no_argument, 0, 'R'},
        {"mmap",  required_argument, 0, 'm'},
        {"sleepTime",  required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    int index = 0;
    while (true) {
        int c = getopt_long (argc, argv, "Rm:s:",
                       long_options, &index);

        if(c == -1) {
            break;
        }

        switch(c) {
            case 'm':
                mmapMinSize = atoi(optarg);
            break;
            case 's':
                sleepTime = atoi(optarg);
            break;
             case 'R':
                recursive = true;
            break;
        }
    }
    if (optind < argc)
    {
        if(optind + 2 > argc) {
            printf("Destination path not specified!\n");
            return false;
        }
    } else {
        printf("Origin path nor Destination paths specified!\n");
        return false;
    }
    return true;
}

bool CheckPaths() {
    bool result = true;
    if(!DirectoryExists(originPath)) {
        printf("Origin path specified does not exist or is not accessible\n");
        result = false;
    }

    if(!DirectoryExists(destinationPath)) {
        printf("Destination path specified does not exist or is not accessible\n");
        result = false;
    }

    return result;
}

bool CanExecute(int argc, char** argv) {
    if(!ParseArguments(argc, argv)) {
        return false;
    }
    
    if(!CheckPaths()) {
        return false;
    }

    return true;
}

void my_handler(int signum)
{
    if (signum == SIGUSR1){
        ReportTrace("Demon has been awakened by SIGUSR1");
        UpdateDirectory(originPath, destinationPath, recursive, mmapMinSize);
    }
}

int main(int argc, char **argv) {
    if(!CanExecute(argc, argv)) {
        return -1;
    }
    
    signal(SIGUSR1, my_handler);
    
    pid_t pid, sid;
    pid = fork();

    if (pid < 0) {
            exit(EXIT_FAILURE);
    }

    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }

    umask(0);
    sid = setsid();

    if (sid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    while (true) {
        ReportTrace("Demon has been awakened from slumber");
        UpdateDirectory(originPath, destinationPath, recursive, mmapMinSize);
        sleep(sleepTime);
    }

    return 0;
}