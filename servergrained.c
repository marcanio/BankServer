
/*Bank Server - Project 2
Author: Eric Marcanio */

#include "servergrained.h"

/*Parameters provided ./server */
int numWorkerThreads;
int numAccounts;
char* outputFileName;

/*File that the user is outputting to*/
FILE *outputFile;

/*Account struct*/
account_t *accounts;

/*Linked list to hold the command line arguments*/
LinkedList_t * commandLine;

/*When END is typed in then stop proccessing threads*/
int end = 1;

/*Added to fix a race condtion with getting new commands*/
pthread_mutex_t commandLock;

/*Used to lock the whole bank rather than accounts*/
pthread_mutex_t bankLock;


int main(int argc, char **argv){
	
	pthread_mutex_init(&commandLock, NULL);
	pthread_mutex_init(&bankLock, NULL);

	int i =0; //loops

	/*Used for the getline*/
	size_t lineMax = 100;
	size_t lineSize = 0;
	
	/*Command type*/
	int id =1;


	/*Actual command*/
	char * command = malloc(1000 * sizeof(char));
	          

	/*Input syntax -> "./appserver <# of worker threads> <# of accounts> <Output file>*/
	if(argc != 4){
		printf("Invalid command-line argument. \n");
		return -1;
	}else{
	/*Take in arguments from the command line */	
		numWorkerThreads = atoi(argv[1]);
		numAccounts = atoi(argv[2]);
		outputFileName = (char *) malloc(sizeof(char *)* 128);
		strcpy(outputFileName, argv[3]);
		outputFile = fopen(outputFileName, "w");
		/*Check if output file is NULL*/
		if(!outputFile){
			printf("Failed to open outputFile\n");
		return -1;
		}	
	}
	
	accounts = malloc(numAccounts * sizeof(account_t));

	if(initAccounts()){
		printf("Problem with account set up");
		return -1;
	}
	
	commandLine = malloc(sizeof(LinkedList_t));
	if(commandLineSetup()){
		printf("Problem with command line set up");
		return -1;
	}
	pthread_t threads[numWorkerThreads];
	
	/*Create worker threads*/
	for(i = 0; i < numWorkerThreads; i++){
		pthread_create(&threads[i], NULL, processRequest , NULL);
	} 

	/*Infinite LOOP starts here for processing */	
	while(1){
		/*Read in line and null terminate*/
		lineSize = getline(&command, &lineMax, stdin);
		command[lineSize -1] = '\0';

		/*The next command added to the list*/
	

		if(strcasecmp(command, "END") == 0){
			/*End the program*/
			end = 0;
			break; //Get out of here!!
		}
		
		printf("ID %d\n", id);
		commandHold_t * newCommand = malloc(sizeof(commandHold_t));
		/*Set up new command*/		
		newCommand->command = malloc(1000 * sizeof(char));
		strncpy(newCommand->command, command, 1000);
		newCommand->attribute = id;
		gettimeofday(&newCommand->timestamp, NULL);
		newCommand->next = NULL;
		
		/*Lock commandLine to maniuplate it*/
		
		
		if(commandLine->size ==0){
			/*If its the first command set to head and tail*/
			commandLine->head = newCommand;
			commandLine->tail = newCommand;
			commandLine->size = 1;
		}else{
			/*If it is not the first add it to the linked list tail*/
			commandLine->tail->next = newCommand;
			commandLine->tail = newCommand;
			commandLine->size = commandLine->size +1;
		}

		/*Done using commandline*/
		
		/*Onto the next id..*/
		id++;	
	}
	
	if(workTime()){
		printf("Problem waiting for command line size");
		return -1;
	}
	
	for(i =0; i < numWorkerThreads; i ++){
		pthread_join(threads[i], NULL);
	}

	free(accounts);
	free(commandLine);
	free(command);
	free_accounts();
	fclose(outputFile);

	return 0;
}
/*Set up the accounts (Mutex init) Lock and value init*/
int initAccounts(){

	int i;

	initialize_accounts(numAccounts);
	/*Create an account for each numAccount*/
	
	for(i =0; i< numAccounts; i++){
		pthread_mutex_init(&accounts[i].lock, NULL);
		accounts[i].value =0;	
	}
	return 0;
}

/*Setting up the command line linked list*/
int commandLineSetup(){
	commandLine->head = NULL;
	commandLine->tail = NULL;
	commandLine->size = 0;

	return 0;
}

/*Give time for worker thread to work on command*/
int workTime(){
	while(commandLine->size > 0){
		usleep(1);
	}
	return 0;
}

/*Sleep to have time to control the thread*/
int workTimeThread(){
	while(commandLine->size == 0 && end){
		usleep(1);
	}
	return 0;
}


/*Process the commands that the user types in*/
void * processRequest(){
	int i =0;

	/*Used to copy over command*/
	commandHold_t curCommand;
	
	/*Temp command to take strtok*/
	char * tempCommand;
	
	/*arrival time of instructions*/
	struct timeval arrivalTime;

	/*Array of arguments from the temp_command*/
	char ** arguments = malloc(21 * sizeof(char*));

	/*Total arguments*/
	int amntArguments =0;
	
	/*Taking in amount from command line*/
	int checkAmount;

	/*Used like a boolean for Insufficient funds*/
	int insufficient =0;

	//Used For grained
	int amount;
	

	while(end || commandLine->size > 0){
		workTimeThread();
		/*Take the next command from linked list*/
		curCommand = newCommand();
		
		/*If there is no command restart. If there is break it up with strtok*/
		if(curCommand.command == NULL){
			continue;
		}else{
			tempCommand = strtok(curCommand.command, " ");
			pthread_mutex_lock(&commandLock);
			while(tempCommand != NULL){
				arguments[amntArguments] = malloc(21 * sizeof(char));
				strncpy(arguments[amntArguments], tempCommand, 21); //Copy over 
				tempCommand = strtok(NULL, " "); //Get the next one
				amntArguments++;
			}
			pthread_mutex_unlock(&commandLock);
			
		}
		
		/*Request to CHECK balance.. Looking for "CHECK #" */
		if(strcasecmp(arguments[0], "CHECK") == 0 && amntArguments == 2){

			/*Added for grained.. LOCK anything accessing account*/
			pthread_mutex_lock(&commandLock);
			amount = atoi(arguments[1]);
			pthread_mutex_unlock(&commandLock);
			
			pthread_mutex_lock(&bankLock);
			/*Read amount inputted & and aat what time*/
			checkAmount = read_account(amount);
			pthread_mutex_unlock(&bankLock);
			

			gettimeofday(&arrivalTime, NULL);

			/*Place info in the output file*/
			flockfile(outputFile);
			
			fprintf(outputFile, "%d BAL %d TIME %ld.%06ld %ld.%06ld\n",
			curCommand.attribute, checkAmount, (long)curCommand.timestamp.tv_sec,
			(long)curCommand.timestamp.tv_usec, (long)arrivalTime.tv_sec,
			(long)arrivalTime.tv_usec);
 
			funlockfile(outputFile);

		}else if(strcmp(arguments[0], "TRANS") == 0 && amntArguments > 1){
			
			/*# of transactions - Subtract TRANS, and divide by 2 bc of account numbers*/
			int transNumber = (amntArguments - 1) / 2;
			
			/*Hold values of account # & account amount*/
			int accountNumber[transNumber];
			int accountAmount[transNumber];

			/*Value of read_account*/
			int readAccount[transNumber];

			/*Added for Grained.. Everything below here access the account*/
			pthread_mutex_lock(&bankLock);

			/*Store accounts and there amounts in temp ints*/
			for(i =0; i < transNumber; i++){
				
				pthread_mutex_lock(&commandLock);
				int iTimes2 = i * 2;
				accountNumber[i] = atoi(arguments[iTimes2 + 1]); 
				accountAmount[i] = atoi(arguments[iTimes2 + 2]);

				pthread_mutex_unlock(&commandLock);


				/*Check for suffiecent funds in each account*/
				readAccount[i] = read_account(accountNumber[i]);
				/*Make sure amount in account is enough*/
				if(accountAmount[i] + readAccount[i] < 0){
					/*Output to file if there is an ISF*/
					gettimeofday(&arrivalTime, NULL);

					/*output info to file*/
					flockfile(outputFile);
					fprintf(outputFile, "%d ISF %d TIME %ld.06%ld %ld.06%ld\n",
					curCommand.attribute, accountNumber[i], 
					(long)curCommand.timestamp.tv_sec, (long) 						curCommand.timestamp.tv_usec,
					(long)arrivalTime.tv_sec, (long)arrivalTime.tv_usec);
					funlockfile(outputFile);

					/*Mark it to know not to execute trans*/
					insufficient = 1; 
					break;
				}
			}
		
			/*Check for insufficient and then execute command*/
			if(insufficient == 0){
				/*Write to the account to complete trans*/
				for(i =0; i < transNumber; i++){
					write_account(accountNumber[i], (accountAmount[i] + 						readAccount[i]));
				}
				/*Print to file*/
				gettimeofday(&arrivalTime, NULL);
				flockfile(outputFile);
				fprintf(outputFile, "%d OK TIME %ld.06%ld %ld.06%ld\n",
				curCommand.attribute, (long) curCommand.timestamp.tv_sec,
				(long) curCommand.timestamp.tv_usec, (long) arrivalTime.tv_sec,
				(long) arrivalTime.tv_usec);
				funlockfile(outputFile);
			}

			pthread_mutex_unlock(&bankLock);
			
		}else{
			fprintf(stderr, "%d INVALID REQUEST FORMAT\n",curCommand.attribute);
		}
		/*Reset boolean*/
		insufficient = 0;

		/*Reset amount of amount of arguments*/
		amntArguments =0;

		free(curCommand.command);
		for(i =0; i < amntArguments; i++){
			free(arguments[i]);
		}
		
	}

	free(arguments);
	return;
}

/*Move to the next command that the user typed in*/
commandHold_t newCommand(){
	/*Moving the head location and getting a return value ready*/
	commandHold_t theNextCommand;

	/*Getting ready to use commandline struct*/	
	

	/*Check for waiting commands*/
	if(commandLine-> size > 0){
		/*Setting the next command equal to the current one in the command line*/
		theNextCommand.attribute = commandLine->head->attribute;
		theNextCommand.command = malloc(1000 * sizeof(char));
		strncpy(theNextCommand.command, commandLine->head->command, 1000);
		theNextCommand.timestamp = commandLine->head->timestamp;
		theNextCommand.next = NULL;
		
		/*Move the head of the command line to next.. temp is used to free mem after change*/
		commandHold_t * temp = commandLine->head;
		commandLine->head = commandLine->head->next;

		free(temp->command);
		free(temp);

		commandLine->size = commandLine->size -1; //Taking command off list
	}else{
		/*Nothing in list*/
		//pthread_mutex_unlock(&commandLine-> lock);
		theNextCommand.command = NULL;
		return theNextCommand;
	}

	
	return theNextCommand;
}



