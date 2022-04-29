#include "helper.h"
#include <stdbool.h>
#include <time.h>

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>

void synchronization(char *originDirectory, char *destinationDirectory)
{
	UpdateDirectory(originDirectory, destinationDirectory, true, 1, 0);
    long timestamp = time(0);
    sleep(1);
    UpdateDirectory(originDirectory, destinationDirectory, true, 1, timestamp);
}

void my_handler(int signum)
{
    if (signum == SIGUSR1){
        syslog (LOG_NOTICE, "SIGUSR1 Signal received.\n");
    }
}

int main (int argc, char *argv[])
{
	if(argc<3){
        perror("\nNot enough arguments. Paths to origin and destination director are required.\n");
        return -1;
	}
	//Przypisanie sciezek z parametrow do zmiennych
	char* originDirectory = argv[1];
	char* destinationDirectory = argv[2];
	//Sprawdzenie czy podane sciezki prowadza do istniejacego pliku/katalogu
	if(access(originDirectory, F_OK) != 0){
		perror("\nFirst path leads to a nonexistent file.\n");
		return -1;
	}
	if(access(destinationDirectory, F_OK) != 0){
		perror("\nSecond path leads to a nonexistent file.\n");
		return -1;
	}
	//Sprawdzenie czy podane parametry sa sciezkami do katalogow
	int type = GetFileType(originDirectory);
	if (type != 2){
		perror("\nFirst path is not a directory.\n");
		return -1;
	}
	type = GetFileType(destinationDirectory);
	if (type != 2){
		perror("\nSecond path is not a directory.\n");
		return -1;
	}
	
	//ustawienie czasu jaki demon bedzie spac (domyslnie 5 minut)
	int timeBeetweenSynchronization = 300;
	if(argc>3){
		char* p;
		timeBeetweenSynchronization = strtol(argv[3], &p, 10);
		if (*p != '\0'){
			perror("\nThe third parameter is not a number..\n");
			return -1;
		}
	}
	
	signal(SIGUSR1, my_handler);
	
	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog ("DEMON_ACTION", LOG_PID, LOG_LOCAL1);
	syslog (LOG_NOTICE, "Synchronization started.\n");
  	
  	while(true)
    {
    	openlog ("DEMON_ACTION", LOG_PID, LOG_LOCAL1);
    	syslog (LOG_NOTICE, "It's time to awaken Demon in loop.\n");
		closelog ();
		
		synchronization(originDirectory, destinationDirectory);
        
        openlog ("DEMON_ACTION", LOG_PID, LOG_LOCAL1);
        syslog (LOG_NOTICE, "Synchronization ended in loop, time for sleep.\n");
        sleep(timeBeetweenSynchronization);
        closelog ();
        //raise(SIGUSR1); //mozliwosc obudzenia demona przez wyslanie sygnalu SIGUSR1 z raise albo pkill -SIGUSR1 -f ./demon
    }
    
    openlog ("DEMON_ACTION", LOG_PID, LOG_LOCAL1);
    syslog (LOG_NOTICE, "Program has ended.\n");
    closelog ();
  	
    return 0;
}
