#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "fcntl.h"

#define BUF_SIZE 128
#define BUILT_INS 3

int readCommand(char* line);
char** splitPipes(char* line,int *pipeProcesses);
char** splitCommand(char* command,int *noOfTokens);
int execute(char** args,int noOfArgs);
int shellExecute(char** args,int noOfArgs,int in_fd,int out_fd);
int executeCd(char** args);
int executeExit(char** args);
int executeHelp(char** args);
void printError(char* errMsg);

//built-in commands for parent process
char* builtIns[] = {
    "cd",
    "exit",
    "help"
};

//functions to execute for built-in commands
int (*builtInFuncs[])(char**)={
    &executeCd,
    &executeExit,
    &executeHelp
};

//print Error message in shell
void printError(char* errMsg)
{
    printf("%s",errMsg);
}

int main()
{
    int status;//status code
    char* line;
    int pipeProcesses=0;//no of commands separated by pipes
    char** pipeCommands;//N commands separated by N-1 pipes
    char** args;        //args to pass for execution
    int noOfTokens;     //no of tokens in one command
    do
    {
        line = (char*)malloc(sizeof(char)*BUF_SIZE);
        printf("\n>>> ");
        status = readCommand(line);
        //split line into commands which are separated by |
        pipeCommands = splitPipes(line,&pipeProcesses);
        if(pipeProcesses==1)
        {
            //split command into tokens
            args = splitCommand(line,&noOfTokens);
            if(args!=NULL&&noOfTokens>0)
                status = execute(args,noOfTokens);
        }
        else if(pipeProcesses>1)
        {
            int i, in_fd = 0;int pipeError=0;
            int FD[2];//store the read and write file descripters
            for(i = 0; i < pipeProcesses - 1; i++)
            {
                if(pipe(FD)==-1)
                {
                    pipeError=1;
                    printError("Error: Error in piping\n");
                    break;
                }
                else
                {
                    args = splitCommand(pipeCommands[i], &noOfTokens);
                    //execute pipe process and write to FD[1]
                    if(noOfTokens>0)
                    {
                        status = shellExecute(args, noOfTokens, in_fd, FD[1]);
                    }
                    else
                    {
                        pipeError=1;
                        printError("Error: Syntax error\n");
                        break;
                    }
                    close(FD[1]);
                    //set the in file descriptor for next command as FD[0]
                    in_fd = FD[0];
                }
            }
            if(!pipeError)
            {
                args = splitCommand(pipeCommands[i], &noOfTokens);
                if(noOfTokens>0)
                {
                    status = shellExecute(args, noOfTokens, in_fd, 1); 
                } 
                else
                {
                    pipeError=1;
                    printError("Error: Syntax error\n");
                    break;
                }  
            }
        }
        free(line);
        free(args);
        fflush(stdin);
        fflush(stdout);
    } while (status==EXIT_SUCCESS);
    //loop runs as long as status is EXIT_SUCCESS
}
//read one line of command from shell
int readCommand(char* line)
{
    char c;
    int position = 0;
    //maximum length of command line
    int bufsize = BUF_SIZE;
    
    while(1)
    {
        c = getchar();
        //read till EOF or newline encountered
        if(c == EOF || c == '\n')
        {
            line[position] = '\0';
            return EXIT_SUCCESS;
        }
        line[position] = c;
        position++;

        if(position >= bufsize)
        {
            //reallocate memory if length of command exceeds bufsize
            bufsize+=BUF_SIZE;
            line = (char*)realloc(line, bufsize);
            if(line == NULL)
            {
                printError("Error: failed to allocate memory\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}
//split one line into piped commands
//pipeProcesses stores no of piped processes
char** splitPipes(char* line,int* pipeProcesses)
{
	*pipeProcesses = 0;
	int bufsize=BUF_SIZE;
    //commands is an array of pipe separated commands
	char** commands=(char**)malloc(sizeof(char*) * BUF_SIZE);
    int lineLength = strlen(line);
    if(commands==NULL)
    {
        printError("Error: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<bufsize;i++)
    {
        commands[i]=(char*)malloc(sizeof(char)*lineLength);
        if(commands[i]==NULL)
        {
            printError("Error: failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
    }
    int j=0;int commandIndex=0;
    unsigned char p = '|';//pipe character
    unsigned char lapo = '\'';//left apostrophe
    unsigned char dapo = '\"';//double apostrophe
    for(int i=0;i<lineLength; )
    {
        //if double quote encountered loop till next double quote
        if(line[i]==dapo)
        {
            commands[commandIndex][j]=line[i];
            i++;j++;
            while(line[i]!=dapo&&i<lineLength)
            {
                commands[commandIndex][j]=line[i];
                i++;j++;
            }
            commands[commandIndex][j]=line[i];
            j++;i++;
        }//if single quote encountered loop till next single quote
        else if(line[i]==lapo)
        {
            commands[commandIndex][j]=line[i];
            i++;j++;
            while(line[i]!=lapo&&i<lineLength)
            {
                commands[commandIndex][j]=line[i];
                i++;j++;
            }
            commands[commandIndex][j]=line[i];
            j++;i++;
        }//if pipe encountered increase commandIndex
        else if(line[i]==p)
        {
            if(strlen(commands[commandIndex])<=0)
            {
                fprintf(stderr,"Error: Syntax error\n");
                *pipeProcesses=0;
                return NULL;
            }
            commandIndex++;
            if(commandIndex>=bufsize)
            {
                bufsize+=BUF_SIZE;
                commands=(char**)realloc(commands,sizeof(char*)*bufsize);
                if(commands==NULL)
                {
                    printError("Error: failed to allocate memory\n");
                    exit(EXIT_FAILURE);
                }
            }
            i++;j=0;
        }//if any other character just add it to currrent command
        else
        {
            commands[commandIndex][j]=line[i];
            i++;j++;
        }
    }

    *pipeProcesses=commandIndex+1;
	commands[*pipeProcesses]=NULL;
	return commands;
}
//split command into tokens 
//which will be used as arguments for calling execvp
char** splitCommand(char* command,int *noOfTokens)
{
    *noOfTokens = 0;
    int isTokenAwk;
    int bufsize=BUF_SIZE;
    int commandLength = strlen(command);
    //tokens is an array of tokens in the command
    char** tokens=(char**)malloc(sizeof(char*) * BUF_SIZE);
    if(tokens==NULL)
    {
        printError("Error: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < BUF_SIZE; i++)
    {
        tokens[i] = (char*)malloc(sizeof(char) * commandLength);
        if(tokens[i]==NULL)
        {
            printError("Error: failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
    }
    int i, j, commIndex = 0;
    int len = strlen(command);
    for(i = 0; i < len; )
    {
        unsigned char sp = ' ';//space
        unsigned char n = '\n';//newline
        unsigned char lapo = '\'';//left apostrophe
        //if any whitespace or carriage return or end of line encountered then increase noOfTokens
        if(command[i]== sp ||command[i]=='\t'||command[i]=='\r'||command[i]=='\a'||command[i]== n)
        {
            if(strlen(tokens[(*noOfTokens)]) > 0)
            {
                (*noOfTokens)++;
            }
            commIndex = 0;
            i++;
        }//if double quote encountered read till next double quote
        else if(command[i]=='\"')
        {
            //argument in double quotes can't be passed to awk command
            if(tokens[(*noOfTokens) - 1][0] == 'a' && tokens[(*noOfTokens) - 1][1] == 'w' && tokens[(*noOfTokens) - 1][2] == 'k')
            {   
                fprintf(stderr,"Error: Syntax error\n");
                return NULL;
            }
            commIndex = 0;
            j = i+1;
            while(j < len && command[j] != '\"')
            {
                tokens[(*noOfTokens)][commIndex] = command[j];
                commIndex++;
                j++;
            }
            if(strlen(tokens[(*noOfTokens)]) > 0)
            {
                (*noOfTokens)++;
            }
            commIndex = 0;
            i = j+1;
        }//if single quote encountered read till next single quote
        else if(command[i]==lapo)
        {
            commIndex = 0;
            j = i+1;
            while(j < len && command[j] != lapo)
            {
                tokens[(*noOfTokens)][commIndex] = command[j];
                commIndex++;
                j++; 
            }
            if(strlen(tokens[(*noOfTokens)]) > 0)
            {
                (*noOfTokens)++;
            }
            commIndex = 0;
            i = j+1;
        }//any other character encountered add to current tokens index   
        else
        {
            tokens[(*noOfTokens)][commIndex] = command[i];
            commIndex++;
            if(i == len - 1)
            {
                if(strlen(tokens[(*noOfTokens)]) > 0)
                {
                    (*noOfTokens)++;
                }
            }
            i++;
        }//reallocate memory if noOfTokens exceed current bufsize
        if((*noOfTokens)>=bufsize)
        {
            bufsize+=BUF_SIZE;
            tokens=(char**)realloc(tokens,sizeof(char*)*bufsize);
            if(tokens==NULL)
            {
                printError("Error: failed to allocate memory\n");
                exit(EXIT_FAILURE);
            }
        }   
    }
    return tokens;
}
//take a command along with its arguments in args
//and execute it in shell
int execute(char** args,int noOfArgs)
{
    if(args[0]==NULL)//exit if no command
        return EXIT_SUCCESS;
    else
    {
        //compare command with a built-in command
        //need to execute separately
        for(int i=0;i<BUILT_INS;i++)
        {
            if(strcmp(builtIns[i],args[0])==0)
            {
                return (*builtInFuncs[i])(args);
            }
        }
        //if no built-in command call shellExecute
        return shellExecute(args, noOfArgs, 0, 1);
    }
}
//execute one command taking command in args
//and input file descriptor in in_fd, output file descriptor in out_fd
int shellExecute(char** args,int noOfArgs,int in_fd,int out_fd)
{
    pid_t pid, wpid;
    int status;
    pid = fork();//create a child process
    char* arg;
    if(pid==0)//Child Process
    {
        //move in_fd to 0(redirect stdin to in_fd)
        if(in_fd!=0)
        {
            dup2(in_fd,0);
            close(in_fd);
        }
        //move out_fd to 1(redirect stdout to out_fd)
        if(out_fd!=1)
        {
            dup2(out_fd,1);
            close(out_fd);
        }
        int mainCommand = 0;int firstSymbolFound=0;
        //mainCommand is the command before any < > or & character
        //firstSymbolFound is flag to check if any < > or & symbol is found or not
        for(int i=0;i<noOfArgs;i++)
        {
            arg=args[i];
            if(!firstSymbolFound)
            {
                if(strcmp(arg,"&")==0||strcmp(arg,"<")==0||strcmp(arg,">")==0)
                {
                    mainCommand=i;//mainCommand length is i
                    firstSymbolFound=1;//first < > or & symbol has been found
                }
            }
            if(strcmp(arg,">")==0)//output redirection
            {
                //open file to write
                int redirect_out_fd = open(args[i+1],O_CREAT | O_TRUNC | O_WRONLY, 0666);
                //redirect stdout to redirect_out_fd
                dup2(redirect_out_fd,STDOUT_FILENO);
            }
            if(strcmp(arg,"<")==0)
            {
                //open file to read from
                int redirect_in_fd = open(args[i+1],O_RDONLY);
                //redirect stdin to redirect_in_fd
                dup2(redirect_in_fd,STDIN_FILENO);
            }
        }
        if(!firstSymbolFound)
            mainCommand=noOfArgs;
        char** args2 = (char**)malloc(sizeof(char*)*(mainCommand+1));
        if(args2==NULL)
        {
            fprintf(stderr,"Error: failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        int i;
        for(i=0;i<mainCommand;i++)
        {
            args2[i]=args[i];
        }
        args2[i]=NULL;
        //execute the command by allocating child process address space & pid 
        //to new process by using execvp
        if(execvp(args2[0],args2)==-1)
        {
            fprintf(stderr,"Error: failed to execute command\n");
            exit(EXIT_FAILURE);
        }
    }//error in forking
    else if(pid<0)
    {
        fprintf(stderr,"Error: error in forking\n");
        exit(EXIT_FAILURE);
    }
    else//parent process
    {
        //wait for completion of child process unless last character is &
        if(strcmp(args[noOfArgs-1],"&")!=0)
        {
            do{
                wpid = waitpid(pid,&status,WUNTRACED);
            }while(!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }
    return EXIT_SUCCESS;
}
//change directory
int executeCd(char** args)
{
    if(args[1]==NULL)
    {
        printError("Error: no argument to cd command\n");
    }
    else
    {
        if(chdir(args[1])!=0)
        {
            printError("Error: directory not found\n");
        }
    }
    return EXIT_SUCCESS;
}
//display help
int executeHelp(char** args)
{
    printf("SHELL HELP\n");
    printf("These are the built-in commands:\n");
    for(int i=0;i<BUILT_INS;i++)
    {
         printf("%s\n",builtIns[i]);       
    }
    printf("Type man <command> to know about a command\n");
    printf("Type man to know about other commands\n");
    return EXIT_SUCCESS;
}
//exit from shell
int executeExit(char** args)
{
    return 1;
}