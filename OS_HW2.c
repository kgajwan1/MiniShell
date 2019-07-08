#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>
#include<stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<signal.h>
#define BUFSIZE 64
#define NORMAL 				11
#define OUTPUT_REDIRECTION 	22
#define INPUT_REDIRECTION 	33
#define PIPELINE 			44
#define BACKGROUND			55
#define INPUT_OUTPUT_REDIRECTION 66
pid_t minish_id;//=getpid();					

enum { MAXLINE = 120 };
int no_of_arguments;
typedef struct process
{
  struct process *next;       
  char **argv;                
  pid_t pid;                  
  pid_t pid_child;
  char completed;             
  char stopped;               
  int status ;                
} process;
process proc[100] ={0};
static int counter=0;

typedef void Sigfunc(int);

Sigfunc *install_signal_handler(int signo, Sigfunc *handler)
{
	struct sigaction act, oact;

	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	if( signo == SIGALRM ) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;  /* SunOS 4.x */
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;  /* SVR4, 4.4BSD */
#endif
	}

	if( sigaction(signo, &act, &oact) < 0 )
		return (SIG_ERR);

	return(oact.sa_handler);
}
void handle_SIGINT()
{
  printf("Found your CTRL-C \n");
  

	if(getpid() != minish_id){
	int i=0;
	while( i<100){
		i++;
		
		if(proc[i].status == 0)
			{
			kill(proc[i].pid_child, SIGTERM);
			}
	}
	}
	else{
	
	}
}
void handle_SIGTSTP()
{
	
  
  	signal(SIGINT, SIG_IGN);
	kill(proc[counter].pid_child, SIGTSTP); /* and send the signal to ourself */
	/* we won't return from the kill until we're continued */
	install_signal_handler(SIGTSTP, handle_SIGTSTP);
		install_signal_handler(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, SIG_DFL); /* reset disposition to default */
	printf("\n%d\n",proc[counter].pid_child);
	return;
}
  
void handle_SIGQUIT()
{
  //printf("Found your exit  \n");
 	//kill;
}


char ** tokenizer2(char *line)
{
  int bufsize = MAXLINE, i = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "Cannot allocate memory\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, "|");
  while (token != NULL) {
    tokens[i] = token;
    i++;
    if (i >= bufsize) {
      bufsize += MAXLINE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL,"|");
  }
  tokens[i] = NULL;
  return tokens;
}

char ** tokenizer1(char *line)
{
  int bufsize = MAXLINE, i = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "Cannot allocate memory\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, " \t\r\n\a");
  while (token != NULL) {
    tokens[i] = token;
    i++;
    if (i >= bufsize) {
      bufsize += MAXLINE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL," \t\r\n\a");
  }
  tokens[i] = NULL;
  return tokens;
}


char ** shifter(char **argv2)
{
 int i=1;
 while(argv2[i]!=NULL)
 {
  if(argv2[i+1]!=NULL)
  {
  strcpy(argv2[i],argv2[i+1]);
  }
  i++;
 }
 argv2[i-1]=NULL;
 i=0;
 return argv2;
}




int checkpipe(char**argv2)
{
  int i=0;
  int mode;
  while(argv2[i]!=NULL)
  {
   i++;
  }  
  i--;
  while(i!=0)
  {
   if (strcmp(argv2[i],"|")==0)
   {
    mode=PIPELINE;
	break;
   }
   i--;
  }
 return mode;
}

int modeSelector(char **argv2)
{
 int i=0;
 int mode;
 int in =0;
 int out=0;
 
  if(checkpipe(argv2)==PIPELINE)
		{
			mode=PIPELINE;
			return mode;
		}
  
  while(argv2[i]!=NULL)
  {
   if(strcmp(argv2[i],"<")==0)
   {
	 in++;
   } 
   if(strcmp(argv2[i],">")==0)
   {
	 out++;
   }
   i++;
  }
  
  if((in!=0) && (out!=0))
  {
   mode=INPUT_OUTPUT_REDIRECTION;
   return mode;
  }
   
  else
  {
	i=0;
	while(argv2[i]!=NULL)
	{
	 if(strcmp(argv2[i],"&")==0)
		{
		   mode=BACKGROUND;
		}
	 else if(strcmp(argv2[i],"<")==0)
		{
			mode=INPUT_REDIRECTION;
			break;
		} 
	 else if(strcmp(argv2[i],">")==0)
		{
			mode=OUTPUT_REDIRECTION;
			break;
		}	
	 else
		{
			mode=NORMAL;
		}
		i++;
    } 
 }
 return mode;
}

void execute(char**argv2,int Mode)
{
  int in,out;
  int mode=Mode;
  pid_t pid1;
  int status1,status2;
  int fd[2];
  int i=0;
  int background_flag=0;
  int io_flag=0;
  while(argv2[i]!=NULL)
  {
   i++;
  }
  if(mode!=BACKGROUND)
  {
  if (strcmp(argv2[i-1],"&")==0)
   {
	background_flag=1;
    i--;
	argv2[i]=NULL;
   }
  }
  else
  {
    if (i>2)
   {
	background_flag=1;
    i--;
	argv2[i]=NULL;
   } 
  }
  proc[counter].pid=getpid();
  pid1 = fork();
  //	mark proc[counter].pid_child=pid1;
  	if(pid1)
	printf("\n [ Pid: %d ] \n",pid1);
  if (pid1 < 0) 
    {
      fprintf(stderr, "fork error: %s", strerror(errno));
      exit(1);
    }
  else if (pid1 == 0) 
    {   
	//install_signal_handler(SIGTSTP, handle_SIGTSTP);
	
  	proc[counter].status=0;    
      switch(mode)
      {
		 case BACKGROUND:
					if(background_flag==0)
					argv2=shifter(argv2);
					proc[counter].status=1;			//1 is background
					break;
					
		 case INPUT_REDIRECTION:	 
					
					
					in = open(argv2[i-1], O_RDONLY);
					
					if(dup2(in,STDIN_FILENO)==-1)
					{
					  perror(" ");
					}
					close(in);
					argv2=shifter(argv2);
					//proc[counter++].status=0;
					break;
					
		 case OUTPUT_REDIRECTION:
					out = creat(argv2[i-1] , 0644) ;
					if(dup2(out,STDOUT_FILENO)==-1)
					{
					  perror(" ");
					}
					close(out);
					argv2=shifter(argv2);
					//proc[counter++].status=0;
					break;
		
		 case INPUT_OUTPUT_REDIRECTION:
					
					in = open(argv2[i-3],O_RDONLY);
					if(dup2(in,STDIN_FILENO)==-1)
					{
					  perror(" ");
					}				
					out = open(argv2[i-1],O_WRONLY|O_CREAT,0666); // Should also be symbolic values for access rights
					if(dup2(out,STDOUT_FILENO)==-1)
					{
					 perror(" ");
					}
					
							close(out);
					      io_flag=1;
									
					break;
		case NORMAL:
					
					break;
	  }	
	  if(io_flag==1)
	  {
	   execlp(argv2[0],argv2[0],argv2[2],NULL);
	   perror(argv2[0]);
		exit(1);
		  }
	  	else 
       		{
	   	 execvp(argv2[0],argv2);
 	  	  perror(argv2[0]);
		exit(1);
       		}
	  
	install_signal_handler(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, SIG_DFL);  
    }
	else 
   {  
      proc[counter].pid_child=pid1;
	
      if((mode==BACKGROUND)|background_flag==1){
		proc[counter].status=1;					//mark 3
  	}else
		{	
			if ( (pid1 = waitpid(pid1, &status1, 0)) < 0) 
			{
             			fprintf(stderr, "waitpid error: %s\n", strerror(errno));
			}
		}
	counter++;
	install_signal_handler(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, handle_SIGTSTP);
	signal(SIGTSTP, SIG_DFL);  
    }
 	
}



void loop_pipe(char **argv2) 
{
  int   p[2];
  char **argv3;
  pid_t pid;
  int   fd_in = 0;
  int background_flag=0;
  int i=0;
  int j=0;
  int in,out;
  int io_flag=0;
  
  
  while (argv2[i] != NULL)
    {
	  argv3=tokenizer1(argv2[i]);
		while(argv3[j]!=NULL)
			{
			  j++;
			}	  
	  int mode = modeSelector(argv3);  
		   if(mode==BACKGROUND)
		    {
			  if (j>2)
				{
					background_flag=1;
					j--;
					argv3[j]=NULL;
				} 
			}
		   
	  if (pipe(p) == -1)
      		{
      			fprintf(stderr,"Pipe failed");
		      }	   

      if (((pid = fork()) == -1)&&(proc[counter].pid_child=pid))
        {
          exit(EXIT_FAILURE);
        }
      else if (pid == 0)
        {
          dup2(fd_in, 0); //change the input according to the old one 
          if (argv2[i+1]!= NULL)
            dup2(p[1], 1);		  
			switch(mode)
          {
		 	    case BACKGROUND:
							if(background_flag==0);
							argv3=shifter(argv3);
							//proc[counter++].status=1;			//1 is background
							break;
				case INPUT_REDIRECTION:	 	
					in = open(argv3[j-1], O_RDONLY);
					if(dup2(in,STDIN_FILENO)==-1)
					{
					  perror(" ");
					}
					close(in);
					argv3=shifter(argv3);
					break;
					
				case OUTPUT_REDIRECTION:
					out = creat(argv3[j-1] , 0644) ;
					if(dup2(out,STDOUT_FILENO)==-1)
					{
					  perror(" ");
					}
					close(out);
					argv2=shifter(argv2);
					break;
		
		 case INPUT_OUTPUT_REDIRECTION:
					
					in = open(argv3[j-3],O_RDONLY);
					if(dup2(in,STDIN_FILENO)==-1)
					{
					  perror(argv3[j-3]);
					}
					out = open(argv3[j-1],O_WRONLY|O_CREAT,0666); // Should also be symbolic values for access rights
					if(dup2(out,STDOUT_FILENO)==-1)
					{
					 perror(argv3[j-1]);
					}
					close(out);
					io_flag=1;	
					break;
	            }	
		  
		  
          close(p[0]);
		  if(io_flag==1)
		  {
		   execlp(argv3[0],argv3[0],argv3[2],NULL);
		   perror(argv3[0]);
		  }
		  else
		  {
		  execvp(argv3[0], argv3);
		  perror(argv3[0]);
		  }
        }
      else
        {
			if(mode==BACKGROUND||background_flag==1)
			proc[counter].status = 1;				//changed mark1
		    
			else{

				proc[counter].status=0;
				wait(NULL);
			 
			}
			close(p[1]);
             fd_in = p[0];
            //save the input for the next command
          
        }
     i++;
	}
}	


int main(void)
{
 char buf1[MAXLINE];
 char buf2[MAXLINE];
 pid_t shell_id=getpid();
minish_id= shell_id;
	//printf("\n%d--------------baap\n",minish_id);
 pid_t pid,pgid,cpid;
 int status;
 char **argv2;
 char **argv3;
 char *command;
 char **arguments;
 int background_flag=0;
 int i=0;
 char current_directory[100];
 install_signal_handler(SIGINT, handle_SIGINT);
 install_signal_handler(SIGTSTP, handle_SIGTSTP);
 install_signal_handler(SIGQUIT, handle_SIGQUIT);
 while (printf("minish>")&&(fgets(buf1, MAXLINE, stdin) != NULL)) 
 {
   
   
   	install_signal_handler(SIGINT, handle_SIGINT);
 	install_signal_handler(SIGTSTP, handle_SIGTSTP);
 	install_signal_handler(SIGQUIT, handle_SIGQUIT);
	
	if(strcmp(buf1,"\n")==0)
	{
	 continue;
	}
   else
   {
     buf1[strlen(buf1) - 1] ='\0';
   strcpy(buf2,buf1); /* null out ?\n? */
   
   argv2=tokenizer1(buf2);
   int i=0;
	
		
   if(strcmp(argv2[0],"exit")==0)
    {
      int i=0;
	printf("\nProcess table 10 entries of 100:\n");
	for(i=0;i<10;i++)
		{
		printf("pid: %d \t pid_child: %d status: %d\n",proc[i].pid,proc[i].pid_child,proc[i].status);
		}
	signal(SIGQUIT,handle_SIGQUIT);
	  exit(0);
    }
	else if(strcmp(argv2[0],"pwd")==0)
    {
	  fflush(stdout);
      printf("\n%s\n",getcwd(current_directory,100));
    }
	else if(strcmp(argv2[0], "cd") == 0)
		{
			if(chdir(argv2[1])==-1)
			{
			 perror("Unable to change");
			}
		}
	else if (strcmp(argv2[0], "fg") == 0)
		{
			// fg code goes here
			//printf("\n %d\n",getpid());
			int i=0, flag=0;
		while( i<  100){
			i++;
			if(proc[i].pid_child != getpid()){
			flag=1;
			break;
			}
		}
			//printf("\n %d\n",proc[i].pid_child);
			if(flag==1){
				//fg(proc[i].pid_child);
				// tcsetpgrp (shell_terminal, proc[i].pid_child);
				 //wait_for_job (getpid());
				//tcsetpgrp (shell_terminal, minish_id);
			}
	
		}
	else if(strcmp(argv2[0], "bg") == 0)
		{
			//bg code goes here
			//printf("\n %d\n",getpid());
			int i=0, flag=0;
		while( i<  100){
			i++;
			if(proc[i].pid_child != getpid()){
			flag=1;
			break;
			}
		}
			//printf("\n %d\n",proc[i].pid_child);
			if(flag ==1 )
				//bg(proc[i].pid_child);
				kill (proc[i].pid_child, SIGCONT);
				//printf("here");
		
		}
    else
	{
	 int mode=modeSelector(argv2);
	 if(mode==PIPELINE)
	  {
		argv3=tokenizer2(buf1); 
		loop_pipe(argv3);
	  }
	  else
	  { 
	   execute(argv2,mode);	 
      }
	 }
	  no_of_arguments=0;
    
 }
}
 return 0;
}
