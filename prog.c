

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/wait.h>

#define BUFFERSIZE 513

typedef struct job {
    int id;
    char name[BUFFERSIZE];
} job;

void getArgs(char** args, char* str, int* num);
void doCdCommand(char** args);
void backgroundCommand(char** args, pid_t* pid, int n, int* numOfJobs, job* jobs);
void nonBackgroundCommand(char** args, pid_t* pid);
void showJobs(int* numOfJobs, job* jobs);
void removeElement(int* numOfJobs, job* jobs, int index);
void killJobs(int* numOfJobs, job* jobs);

#define TRUE 1

char lastPath[PATH_MAX] = "\0";
char tempCommand[BUFFERSIZE] = "\0";

int main(int argc, char* argv[]) {
    char buffer[BUFFERSIZE] = "\0";
    char* str;
    char* args[BUFFERSIZE];
    pid_t pid;
    
    getcwd(lastPath, sizeof(lastPath));
    
    job jobs[512];
    int numOfJobs = 0;
    
    printf("> ");
    while(TRUE)
    {
        fgets(buffer, BUFFERSIZE , stdin);
        int x;
        // check where is x located
        for(x=0; x<=BUFFERSIZE; x++) { if( buffer[x] == '\n') { break; } }
        int num = 0;
        if (x != 0) {
            str = strtok(buffer, "\n");
            strcpy(tempCommand, str);
            getArgs(args, str, &num);
        }
        if (num > 0) { // if there are args
            if (strcmp(args[0], "cd") && strcmp(args[0], "exit") && strcmp(args[0], "jobs")) {
                if (strcmp(args[num - 1], "&")) { 
                    nonBackgroundCommand(args, &pid);
                } else {
                    backgroundCommand(args, &pid, num, &numOfJobs, jobs);
                }
            } else { // if it is cd or exit or jobs commands
                // build in command
                if (!strcmp(args[0], "cd")) {
                    doCdCommand(args);
                    
                } else if (!strcmp(args[0], "exit")) {
                    printf("%d\n",(int)getpid());
                    killJobs(&numOfJobs, jobs);
                    exit(0);
                    
                } else if (!strcmp(args[0], "jobs")) {
                    showJobs(&numOfJobs, jobs);
                }
            }
        }
        printf("> ");
    }
    return 0;
}


/******************
* Function Name: removeElement
* Input: int* (the number of jobs)
*        job* (the jobs)
*        int (the index to remove)
* Function Operation: the function remove from
*                     the jobs array the job
*                     in the index
******************/
void removeElement(int* numOfJobs, job* jobs, int index) {
    int i;
    for (i = index + 1; i < *numOfJobs; i++) {
        jobs[i-1] = jobs[i];
    }
    *numOfJobs = *numOfJobs - 1;
}

/******************
* Function Name: killJobs
* Input: int* (the number of jobs)
*        job* (the jobs)
* Function Operation: kill the remaning jobs
******************/
void killJobs(int* numOfJobs, job* jobs) {
    int i;
    int stat;
    for (i = 0; i < *numOfJobs; i++) {
        if (!waitpid(jobs[i].id, &stat, WNOHANG)) {  // is alive
            kill(jobs[i].id, 9);
        }
    }
}

/******************
* Function Name: showJobs
* Input: int* (the number of jobs)
*        job* (the jobs)
* Function Operation: show the jobs that running
******************/
void showJobs(int* numOfJobs, job* jobs) {
    int i;
    int stat;
    for (i = 0; i < *numOfJobs;) {
        if (!waitpid(jobs[i].id, &stat, WNOHANG)) {  // is alive
            printf("%d %s\n", jobs[i].id, jobs[i].name);
            i++;
        } else {
            removeElement(numOfJobs, jobs, i);
        }
    }
}


/******************
* Function Name: backgroundCommand
* Input: char** (the args of the command)
*        pid_t* (the pid of the process)
*        int  (the number of args)
*        int* (the number of jobs)
*        job* (the jobs)
* Function Operation: run the background command
******************/
void backgroundCommand(char** args, pid_t* pid, int n, int* numOfJobs, job* jobs) {
    char* tempArgs[BUFFERSIZE];
    char name[BUFFERSIZE] = "\0";
    int i;
    for (i = 0; i < n - 1; i++) {
        tempArgs[i] = args[i];
        strcat(name, args[i]);
        if (i < n - 2) {
           strcat(name, " ");
        }
    }
    tempArgs[n - 1] = NULL;
    
    int stat;
    
    if (((*pid) = fork()) == 0) {
        // son code
        
        execvp(args[0], tempArgs);
        fprintf(stderr, "Error in system call\n");
        exit(1);
        
    } else if ((*pid) < 0) {
        fprintf(stderr, "Error in system call\n");
        
    } else {
        printf("%d\n", (*pid));
        job temp;
        
        temp.id = (int)(*pid);
        strcpy(temp.name, name);
        jobs[*numOfJobs] = temp;
        *numOfJobs = 1 + *numOfJobs;
    }
}

/******************
* Function Name: nonBackgroundCommand
* Input: char** (the args of the command)
*        pid_t* (the pid of the process)
* Function Operation: run the non background command
******************/
void nonBackgroundCommand(char** args, pid_t* pid) {
    int stat;
    
    if (((*pid) = fork()) == 0) {
        // son code
        printf("%d\n",(int)getpid());
        execvp(args[0], args);
        fprintf(stderr, "Error in system call\n");
        exit(1);
            
    } else if((*pid) > 0) {
        // parent code
        int id = (int)(*pid);
        waitpid(id, &stat, 0);
    } else { // if ((*pid) < 0)
        fprintf(stderr, "Error in system call\n");
    }
}

/******************
* Function Name: doCdCommand
* Input: char** (the args of the command)
* Function Operation: run the CD command
******************/
void doCdCommand(char** args) {
    char path[BUFFERSIZE] = "";
    char temp[PATH_MAX] = "\0";
    char tempPath[PATH_MAX] = "\0";
    int val;
    int i = 1;
    
    getcwd(temp, sizeof(temp));
    
    printf("%d\n",(int)getpid());
    if (args[1] == NULL) { // if only "cd"
        val = chdir(getenv("HOME"));
        
    } else if(!strcmp(args[1], "-")) { // do cd -
        val = chdir(lastPath);
        
    } else {
        // build the path
        if (!strncmp(args[1], "\"", 1)) {  // the path is: "the_path"
            char* token = strtok(tempCommand, "\"");
            token = strtok(NULL, "\"");
            
            if (token == NULL) {  // invalid input
                return;
            }
            
            strcpy(path, token);
        
        } else {  // the path dont have '"'
            strcpy(path, args[1]);
        }
        
        strcpy(tempPath, path);
        char* str = strtok(tempPath, "/");
        if (!strncmp(str, "~", 1)) {
            // make the absolote path
            char fullPath [BUFFERSIZE] = "";
            strcat(fullPath, getenv("HOME"));
            strcat(fullPath, (path+1));
            val = chdir(fullPath);
            
        } else {
            val = chdir(path);
        }
    }
    
    if (!val) {  // made the cd witout fail
        strcpy(lastPath, temp);
    }
}

/******************
* Function Name: getArgs
* Input: char** (the args of the command)
*        char*  (the user 
* Function Operation: run the CD command
******************/
void getArgs(char** args, char* str, int* num) {
    char* token = strtok(str, " ");
        
    int n = 0;
    while (token != NULL) { 
        args[n] = token;
        n++;
        token = strtok(NULL, " "); 
    }
    args[n] = NULL;
    *num = n;
}
