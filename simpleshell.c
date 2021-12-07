#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/* linked-list of jobs */
typedef struct job
{
    int num;
    pid_t pid;
    int status;    //fg:1, bg:0
    char *command; //the command string
    struct job *next;
} job;

//int size = 0;
job *head = NULL;
job *curr = NULL;
pid_t shellpid = 0;

/* adds (background) job to a linked-list */
void addJob(char *args[], pid_t *pid)
{
    job *job = malloc(sizeof(*job));
    //printf("malloc \n");
    if (head == NULL)
    { // new job
        job->num = 1;
        job->pid = *pid;
        job->next = NULL;
        job->status = 0; //background by default
        job->command = args[0];
        head = job;
        curr = job;
        //printf("job added \n");
        //printf("%s\n", job->command);
    }
    else
    {
        job->num = curr->num + 1;
        job->pid = *pid;
        job->next = NULL;
        job->command = args[0];
        curr->next = job;
        curr = job;
    }
    //job->command = args;
    //size++;
}

pid_t getJob(int pNum)
{
    job *tmp = head;
    //int num = 0;
    while (tmp != NULL)
    {
        if (tmp->num == pNum)
        {
            curr = tmp;
            shellpid = tmp->pid;
            return tmp->pid; // return the job if it is in job list
        }
        tmp = tmp->next;
    }
    return -1; // return -1 if the job dne
}

char *jobstatus(int status)
{
    if (status == 1)
    {
        return "FG";
    }
    return "BG";
}

void jobs()
{
    job *curr = head;
    int status;
    //int num = 0;
    printf("Printing jobs... \n");
    printf("Job ID \t\t Process ID \t\t Status \n");
    while (curr != NULL)
    {
        //printf("while loop \n");
        printf("[%d] \t\t pid:%d \t\t %s \n", curr->num, curr->pid, jobstatus(curr->status));
        //num++;
        curr = curr->next;
    }
}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0)
    {
        exit(-1);
    }

    if ((loc = index(line, '&')) != NULL)
    {
        //printf("background job \n");
        *background = 1;
        *loc = ' ';
    }
    else
    {
        *background = 0;
    }
    char *line2 = line;
    while ((token = strsep(&line2, " \t\n")) != NULL)
    {
        for (int j = 0; j < strlen(token); j++)
        {
            if (token[j] <= 32)
            {
                token[j] = '\0';
            }
        }
        if (strlen(token) > 0)
        {
            args[i++] = token;
        }
    }
    free(line2);
    return i;
}

/* built-in commands */

void cd(char *args[])
{
    int res = chdir(args[1]);
    if (res != 0)
    {
        printf("cd error \n");
        //return;
    }
    return;
}

void pwd()
{
    char cwd[200];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        //fprintf(stderr, "pwd() error. \n");
        printf("pwd()error. \n");
        //return -1;
    }
    else
    {
        printf("Current directory: %s \n", cwd);
        //return 1;
    }
    //return 0;
}

void fg(char *args[])
{
    int status;
    int jobnum = atoi(args[1]);

    int fg = waitpid(getJob(jobnum), &status, WNOHANG);
    if (fg == -1)
    {
        printf("forking error \n");
        exit(EXIT_FAILURE);
    }
    else
    {
        while (curr != NULL)
        {
            if (curr->num == jobnum)
            {
                curr->status = 1; // set status to fg
            }
            curr = curr->next;
        }
        printf("fg complete \n");
    }
    //return 0;
}
// returns 1 if the command line arguments contain a |
int doPipe(char *args[])
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            return 1;
        }
    }
    return -2;
}
// returns 1 if the command line arguments contain a >
int doRedirection(char *args[])
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], ">") == 0)
        {
            return 1;
        }
    }
    return -2;
}

int array_length(char *args[])
{
    int count = 0;
    for (int i = 0; args[i] != 0; i++)
    {
        count++;
    }
    return count;
}

int split_delimiter(char *args[], char *delimiter)
{
    int words = array_length(args);
    int index = -1;
    for (int i = 0; i < words; i++)
    {
        if (strcmp(args[i], delimiter) == 0)
        {
            index = i;
        }
    }
    return index;
}

void command_piping(char *args[])
{
    int split_index = split_delimiter(args, "|");
    int arr_index = array_length(args);
    char *parent[split_index + 1];
    char *child[arr_index - split_index + 1];
    int i, j, k;
    for (i = 0; i < split_index; i++)
    {
        parent[i] = args[i];
    }
    parent[split_index] = NULL;
    //parent[split_index+1]=NULL;
    //printf("done splitting parent \n");
    for (j = 0, k = split_index + 1; k < arr_index; j++, k++)
    {
        child[j] = args[k];
    }
    child[arr_index - split_index] = NULL;
    //child[arr_index-split_index+1]=NULL;

    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid < 0)
    {
        printf("error, can't fork process. \n");
    }
    if (pid > 0)
    {
        close(pipefd[1]);
        dup2(pipefd[0], 0);
        close(pipefd[0]);
        printf("execvp child\n");
        execvp(child[0], child);
    }
    else
    {
        //connect the pipe
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        printf("execvp parent\n");
        execvp(parent[0], parent);
    }
}

void output_redirection(char *args[])
{
    // > output of the command is directed to the file
    int split_index = split_delimiter(args, ">");
    int arr_index = array_length(args);
    char *parent[split_index + 1];
    char *file;
    int i;
    for (i = 0; i < split_index; i++)
    {
        parent[i] = args[i];
    }
    parent[i] = NULL;
    strcpy(file, args[split_index + 1]);
    //printf("filename: %s\n",file);
    char *parent_cmd = parent[0];
    //int output = creat(file, 0644);
    int standout = dup(1);
    close(1);
    int output = open(file, O_WRONLY | O_APPEND | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
    //printf("%d\n", output);
    //dup2(output, STDOUT_FILENO);
    //close(output);
    execvp(parent_cmd, parent);
}

int init_args(char *args[])
{
    for (int i = 0; i < 20; i++)
    {
        args[i] = NULL;
    }
    return 0;
}

static void sigint_handler(int sig)
{
    //signal(SIGINT, SIG_DFL); // catches ctrl c and kills process
    //signal(sig, SIG_IGN);
    if (getpid() != shellpid)
    {
        printf("\n Ctrl + C: killing process... \n");
        exit(EXIT_FAILURE);
    }
    //exit(0);
}

static void sigtstp_handler(int sig)
{
    printf("\n CTrl + Z: do nothing \n");
    signal(SIGTSTP, SIG_IGN);
}

void freeMemory()
{
    struct job *current = head;
    while (current != NULL)
    {
        struct job *temp = current;
        current = current->next;
        free(temp);
    }
}

int main()
{
    char *args[20];
    int bg;
    int status;
    pid_t pid;
    while (1)
    {
        bg = 0;
        //shellpid = fork();
        init_args(args);
        char *line = NULL;
        int cnt = getcmd("\n>>  ", args, &bg);
        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, SIG_IGN); // ignore ctrl Z
        //signal(SIGTSTP,sigtstp_handler);
        if (cnt == -1)
        {
            fprintf(stderr, "can't parse command \n");
            continue;
        }
        if (args[0] == NULL)
        {
            continue;
        }
        /* execute builtin commands */
        if (strcmp(args[0], "cd") == 0)
        {
            if (args[1] == NULL)
            {
                printf("No path entered. Printing current directory... \n");
                pwd();
            }
            else
            {
                cd(args);
                //return 1;
            }
        }
        else if (strcmp(args[0], "pwd") == 0)
        {
            printf("doing pwd \n");
            pwd();
            //return 1;
        }
        else if (strcmp(args[0], "exit") == 0)
        {
            printf("Exiting shell \n");
            freeMemory();
            exit(0);
            //return 1;
        }
        else if (strcmp(args[0], "jobs") == 0)
        {
            jobs();
            //return 1;
        }
        else if (strcmp(args[0], "fg") == 0)
        {
            if (args[1] == NULL)
            {
                printf("fg requires a job number \n");
            }
            else
            {
                fg(args);
                //return 1;
            }
        }
        // else if (doRedirection(args)==1){
        //     printf("redirecting... \n");
        //     int redir = output_redirection(args);
        //     if (redir < 0){
        //         printf("error execvp inside redirection \n ");
        //     }
        //     //fflush(stdout);
        // }
        // else if (doPipe(args)==1){
        //     printf("piping... \n");
        //     command_piping(args);
        // }
        /* other executable commands  */
        else
        {
            //printf("other commands \n");
            pid = fork();
            if (pid < 0)
            {
                printf("error forking \n");
            }
            else if (pid == 0)
            {
                //delay
                //printf("parent \n");
                //sleep(5);
                if (doRedirection(args) == 1)
                {
                    printf("redirecting... \n");
                    output_redirection(args);
                }
                else if (doPipe(args) == 1)
                {
                    printf("piping... \n");
                    //command_piping(args);
                    int split_index = split_delimiter(args, "|");
                    int arr_index = array_length(args);
                    char *parent[split_index + 1];
                    char *child[arr_index - split_index + 1];
                    int i, j, k;
                    for (i = 0; i < split_index; i++)
                    {
                        parent[i] = args[i];
                    }
                    parent[i] = NULL;
                    //parent[split_index+1]=NULL;
                    //printf("done splitting parent \n");
                    for (j = 0, k = split_index + 1; k < arr_index; j++, k++)
                    {
                        child[j] = args[k];
                    }
                    child[j] = NULL;
                    //child[arr_index-split_index+1]=NULL;
                    //printf("first cmd: %s\n", parent[0]);
                    //printf("second cmd: %s\n", child[0]);
                    int pipefd[2];
                    pipe(pipefd);
                    pid_t pipepid = fork();
                    if (pipepid == -1)
                    {
                        printf("error, can't fork process. \n");
                    }
                    if (pipepid > 0)
                    {
                        
                        dup2(pipefd[0], 0);
                        close(pipefd[1]);
                        //close(pipefd[0]);
                        execvp(child[0], child);
                        //printf("execvp second\n");
                    }
                    else
                    {
                        //connect the pipe
                        dup2(pipefd[1], 1);
                        close(pipefd[0]);
                        execvp(parent[0], parent);
                        //printf("execvp first\n");
                    }
                }
                else
                {
                    execvp(args[0], args);
                }
            }
            else
            {

                if (bg == 0)
                {
                    int status;
                    shellpid = pid;
                    int wait = waitpid(pid, &status, WUNTRACED);
                    if (wait == -1)
                    {
                        printf("error waitpid \n");
                    }
                }
                else
                {
                    printf("adding job... \n");
                    addJob(args, &pid);
                    printf("background job added \n");
                }
            }
        }
        // if (bg == 0){
        //     if (shellpid<0){
        //         printf("error forking \n");
        //     }
        //     else if (shellpid > 0){
        //         waitpid(shellpid,&status,0);
        //     }
        //     else{
        //         if (doPipe(args)==1){
        //             printf("doing command piping \n");
        //         }
        //         else if (doRedirection(args)==1){
        //             printf("output redirection \n");
        //             pid_t child = fork();
        //             int status;
        //             if (child == -1){
        //                 printf("error forking \n");
        //             }
        //             else if (child == 0){
        //                 output_redirection(args);
        //                 printf("performing output redir \n");
        //             }
        //             else{
        //                 pid_t pid = waitpid(child,&status,0);
        //                 printf("wait \n");
        //             }
        //         }
        //     }
        // }
        // else if (doPipe(args)){
        //     command_piping();
        // }

        free(line);
        fflush(stdout);
    }
    return 0;
}
