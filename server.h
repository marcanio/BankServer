#include "Bank.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>


/*Storess mutex locks for each account*/
typedef struct accountStruct{
	pthread_mutex_t lock;
	int value;
}account_t;

//Struct used for commands
//Actual commmand, id, timestamp, next attribute
typedef struct commandStruct{
	char * command;
	int attribute;
	struct timeval timestamp;
	struct commandStruct * next; //Pointer to access another piece of data
}commandHold_t;

//Use a linked list sturct of type lineked cpmmand as head and tail
typedef struct LinkedListStruct{
	int size;
	pthread_mutex_t lock;
	commandHold_t * head;
	commandHold_t * tail;
}LinkedList_t;

/*Sets commandLine to the next command*/
commandHold_t newCommand();

/*Set up the accounts (Mutex init)*/
int initAccounts();

/*Setting up linked list for command lines*/
int commandLineSetup();

/*This will process all the commands the user types in */
void * processRequest();

/*usleep is used to give time to worker threads*/
int workTime();

/*usleep used to give time for threads*/
int workTimeThread();

