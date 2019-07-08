#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/resource.h>
#include <fcntl.h>
#define DELIMS " \t\r\n\a"

void removeProcFromList(pid_t pid);       
bool isProcThere(int pid);
int findPidToRemove(int pid);
int runPipeExec(char** args);

pid_t pid_list[1000];
int pid_list_index = -1;

void CtrlCHandler() 
{
	printf("\n");
	fflush(stdout);
	return;
}

void procFinishHandler(int sig)
{
	int pid;
	pid = wait(NULL);
	if (pid != -1) {
		int temp = findPidToRemove(pid);
		pid_list[temp] = -1;
		return;
	}
}



int runListJobs() 
{
	if (pid_list_index == -1) {
		printf("No background processes running\n");
		return 0;
	}

	printf("List of backgrounded processes:\n");

	int i = 0;
	int status;
	int toRemove[20]; // if exit at same time

	for (int i = 0; i <= pid_list_index; i++) 
	{
		printf("Command %d with PID %d", i+1, pid_list[i]);
		if (pid_list[i] == -1) {
			printf(" Status:FINISHED\n");
			removeProcFromList(pid_list[i]);
			return 0;
		}

		pid_t ret = waitpid(pid_list[i], &status, WNOHANG);
		
		if (ret == -1) {
			printf(" Status:ERROR\n");
			exit(1);
		} else if (ret == 0) {
			printf(" Status:RUNNING\n");
		} else {
			printf(" Status:FINISHED\n");
			toRemove[i] = ret;
			i++;
		}
	}

	while (toRemove[i] != 0) 
	{
		removeProcFromList(toRemove[i]);
		i++;
	}

	return 0;
}

int findPidToRemove(int pid)
{
	int remove = -1;
	for (int i = 0; i <= pid_list_index; i++) {
		if (pid == pid_list[i]) {
			remove = i;
		}
	}
	return remove;
}

void removeProcFromList(pid_t pid)
{
	int remove = -1;
	for (int i = 0; i <= pid_list_index; i++) {
		if (pid == pid_list[i]) {
			remove = i;
		}
	}
	// iterate from index and shift remaining
	for (int i = remove; i <= pid_list_index; i++) {
		pid_list[i] = pid_list[i+1];
	}
	pid_list_index--;
}

void cleanupProcesses() 
{
	int temp_pid;
	for (int i = 0; i <= pid_list_index; i++) {
		temp_pid = pid_list[i];
		printf("KILL PID: %d\n", temp_pid);
		kill(temp_pid, SIGKILL);
	}
}

void runExec(char **args, bool background) 
{
	if (args[0] == NULL) 
	{
	return;
	}

	int pid;
	int pstatus, cstatus;
	pid = fork();
	if (pid == -1) 
			{perror("fork() failed in runExec()\n");
			exit(-1);}

	else if (pid == 0) 
			{
				if (execvp(args[0], args) == -1) {
				printf("Failed to execute command\n");
				exit(-1);
			}	
			}
	else{
			if (background) {
				pid_list_index++;
				pid_list[pid_list_index] = pid; 
			} else { 
				if(waitpid(pid, &pstatus, 0)== -1)
				{
				printf("Error");	
				exit(1);
				}
			}
	}

	return;
}

char ** parseLine(char *line) 
{
	int buffer = 25;
	char **args = malloc(buffer * sizeof(char*));
	char *arg;
	int pos = 0;

	if (!args) {
		perror("Mem alloc error - args\n");
		exit(1);
	}

	arg = strtok(line, DELIMS);
	
	while (arg != NULL) {
		args[pos] = arg;
		pos++;
		// check if need more mem
		if (pos >= buffer) {
			buffer += 25;
			args = realloc(args, buffer * sizeof(char *));
			if (!args) {
				perror("Mem realloc error - args");
				exit(2);
			}
		}
		
		arg = strtok(NULL, DELIMS); 
	}
	
	args[pos] = NULL; 
	return args;
}

bool isProcThere(int pid) {
	for (int i = 0; i <= pid_list_index; i++) {
		if (pid == (int) pid_list[i]) {
			return true;
		}
	}
	return false;
}



void makeFg(char** args)
{
	// convert pid string to int
	int pid = (int)strtol(args[1], (char **)NULL, 10);

	if (!isProcThere(pid)) {
		fprintf(stderr, "PID is not in background\n");
		return;
	}	

	if(waitpid(pid, NULL, 0)== -1)
		{
		printf("Error here");
			exit(1);}
	removeProcFromList(pid);
	return;
}


bool isBg(char** args)
{
	int i = 0;
	while(args[i] != NULL) {
		if (strcmp(args[i], "&") == 0) {
			args[i] = NULL;
			return true;
		}
		i++;
	}
	return false;
}


int runRedirectExecIn(char** args)
{
	int i;

	char** command = args;
	char file[50];
	int cmd_len = 0;

	i = 0;
	while(args[i] != NULL)
	{
		if (strcmp(args[i], "<") == 0)
		{
			cmd_len = i;
			strcpy(file, args[i+1]);
			break;
		}
		i++;
	}

	FILE * fp;
    	char * line = NULL;
    	size_t len = 0;
    	ssize_t read;

    fp = fopen(file, "r");
    if (fp == NULL)
        {printf("file not present \n");
          //exit(1);
          return -1;
	}
              
    int count =0;
    while ((read = getline(&line, &len, fp)) != -1) {
      
       count++;
       
    }
    printf("%d\n",count);
    fclose(fp);

    strcpy(command[cmd_len],line);
    command[cmd_len+1] = NULL;

    if (line)
        free(line);
    
    runExec(command, false);		
}


int runRedirectExecOut(char** args)
{
	int i;
	pid_t pid;
	char** command = args;
	char file[50];
	char output[4096];
	int link[2];
	FILE* fp;

	i = 0;
	while(args[i] != NULL)
	{
		if (strcmp(args[i], ">") == 0)
		{
			command[i] = NULL;
			strcpy(file, args[i+1]);
			break;
		}
		i++;
	}

	if (pipe(link)==-1)
    perror("pipe");

  if ((pid = fork()) == -1)
    perror("fork");

  if(pid == 0)
  {

    dup2 (link[1], STDOUT_FILENO);
		if (pid == -1)
			{
			
    perror("dup2 failed");

			}
    close(link[0]);
    close(link[1]);
    execvp(command[0], command);
    perror("execvp");

  } 
  else 
  {

    close(link[1]);
    int nbytes = read(link[0], output, sizeof(output));
    waitpid(pid, NULL, 0);
    fp = fopen(file, "w");
    fprintf(fp, "%.*s\n", nbytes, output);
    fclose(fp);

  }
		
}

bool isPipe(char** args)
{
	int i = 0;
	while(args[i] != NULL) 
	{
		if ((strcmp(args[i], "|") == 0))
		{
			return true;
			//runPipeExec(args);
		}
		i++;
	}
	return false;
}


bool isRedirectIn(char** args)
{
	int i = 0;
	while(args[i] != NULL) 
	{
		if ((strcmp(args[i], "<") == 0))
		{
			return true;
		}
		i++;
	}
	return false;
}

bool isRedirectOut(char** args)
{
	int i = 0;
	while(args[i] != NULL) 
	{
		if ((strcmp(args[i], ">") == 0))
		{
			return true;
		}
		i++;
	}
	
	return false;
}


int runPipeExec(char** args)
{
	int filedes[2]; 
	int filedes2[2];
	int num_cmds = 0;
	char *command[256];
	pid_t pid;
	int err = -1;
	int end = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	while (args[j] != NULL && end != 1){
		k = 0;
		
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				end = 1;
				k++;
				break;
			}
			k++;
		}
		command[k] = NULL;
		j++;		
		
		if (i % 2 != 0){
			pipe(filedes); // for odd i
		}else{
			pipe(filedes2); // for even i
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return 1;
		}
		if(pid==0){
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			if (i == -1){
				printf("dup2 Failed");
			}

			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ 
					dup2(filedes2[0],STDIN_FILENO);
				}
			
			}else{ 
				if (i % 2 != 0)
				{
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ 
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}
				
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
		

}

int main(int argc, char** argv)
{
	char **args;
	char *line = NULL;
	pid_t pid;
	char *inputFile, *outputFile;
	size_t buffer = 1024;
	int i = 0;

	while(1)
	{
		printf("minish> ");

		signal(SIGCHLD, procFinishHandler);
		
		signal(SIGINT, CtrlCHandler);

		getline(&line, &buffer, stdin);
		args = parseLine(line);

		if (*args[0] == '\r' || *args[0] == '\n') {
			continue;
		}
		else if (strcmp(args[0], "exit") == 0) { 
			
			cleanupProcesses();
			printf("Exiting Shell\n");
			exit(0);
		}
		else if (strcmp(args[0], "jobs") == 0) {
 			//replace this by ps for best results
			runListJobs();
			continue;
		} 
		else if ((strcmp(args[0], "fg") == 0)) {

			if (args[2] != NULL) {
				printf("fg command takes one argument - pid number\n");
				continue;
			} else {
				makeFg(args);
			}	
		}
		
		
		else if (isRedirectIn(args) == 1){
			runRedirectExecIn(args);	
		}
		
		else if (isRedirectOut(args) == 1){
			runRedirectExecOut(args);	
		}

		else if (isPipe(args) == 1){
			printf("Here pipe  \n");
			runPipeExec(args);
		}
			
		
		else if (args[0] != NULL) {
			runExec(args, isBg(args));
			
		}
		
		free(line);
		free(args);
		line = NULL;
		args = NULL;
	}
	return 0;
}