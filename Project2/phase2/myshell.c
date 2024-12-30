/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128
#define NONE 0
#define RUNNING_FG 1
#define RUNNING_BG 2
#define SUSPENDED 3
#define UNDEFINED 4

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv,int pipe_flag);
void parsepipe(char *cmdline);
void Pipe_eval(char *cmds[],int num);
int builtin_command(char **argv); 
void del_char(char *cmdline);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

typedef struct jobs{
    pid_t pid;
    int id;
    char cmd[MAXLINE];
    int state;
} jobs;

jobs jobs_list[MAXARGS];
volatile int num_jobs;
int command_flag;
int pipe_flag=0;
void print_jobs(void);
void add_job(pid_t pid,char *cmd, int state);
void clear_job(int idx);
void kill_job(int idx);
int find_idx(pid_t pid);
int fg_check(void);

volatile sig_atomic_t realpid;
/*
0411 : ctrl + c 가 안 먹힐 때가 있음. 
*/
int main() 
{
    // sigset_t mask_all,mask_one,prev_one;
    char cmdline[MAXLINE]; /* Command line */

    // Sigfillset(&mask_all);
    
    Signal(SIGINT,sigint_handler);
    Signal(SIGTSTP,sigtstp_handler);
    Signal(SIGCHLD,sigchld_handler);
    
    while (1)
    {
        command_flag = 0;
	    /* Read */
	    printf("CSE4100-SP-P2> ");                   
	    fgets(cmdline, MAXLINE, stdin); 
        del_char(cmdline);

        command_flag = 1;

        if (feof(stdin))
        {
            exit(0);
        }
	    /* Evaluate */
        if(strchr(cmdline,'|')) 
            parsepipe(cmdline);
        else
	        eval(cmdline);
    } 
}
/* $end shellmain */
void parsepipe(char *cmdline) 
{
    //char buf[MAXLINE];
    int pipe_count = 0;
    char *commands[MAXARGS];
    char *next_cmd;
    int bg;
    int in_fd,out_fd;
    int fds[MAXARGS][2]; // 각 파이프에 대한 파일 디스크립터 저장


    next_cmd = strtok(cmdline, "|");
    while (next_cmd != NULL) {
        commands[pipe_count++] = next_cmd;
        next_cmd = strtok(NULL, "|");
    }

    pipe_count--;

    Pipe_eval(commands,pipe_count);
}
void Pipe_eval(char *cmds[],int num)
{
    char buf[MAXLINE];
    int i,j, bg, idx;
    // pid_t pid;
    sigset_t p_mask, c_mask, mask_all, prev_mask;
    // cmdline[n][m] n번째 argv의 m번째 를 가리키는 ~
    char ***argv = (char ***)malloc(sizeof(char **) * (num + 1));
    
    for (i = 0; i <= num; i++) 
    {
        argv[i] = (char **)malloc(sizeof(char *) * (MAXARGS/num));
        
        for (j = 0; j < (MAXARGS/num); j++) 
        {
            argv[i][j] = (char *)malloc(sizeof(char) * (MAXLINE/num));
        }
    }

    int **fds = (int **)malloc(sizeof(int *) * num);
    
    for (i = 0; i < num; i++) 
    {
        fds[i] = (int *)malloc(sizeof(int) * 2);
    }

    for (i = 0; i < num; i++) 
    {
        if(pipe(fds[i]) == -1)
            fprintf(stderr,"error");
    }

    Sigfillset(&mask_all);
    Sigemptyset(&p_mask);
    Sigaddset(&p_mask,SIGCHLD);
    Sigaddset(&p_mask,SIGINT);
    Sigaddset(&p_mask,SIGTSTP);
    Sigemptyset(&c_mask);
    Sigaddset(&c_mask,SIGINT);
    Sigaddset(&c_mask,SIGTSTP);

    for (i = 0; i < num + 1; i++) 
    {
        // if(i != num) pipe_flag = 1;
        // else pipe_flag = 0;
        strcpy(buf,cmds[i]);
        bg = parseline(buf,argv[i],1);
		// printf("pipe_bg : %d\n",bg);
        if(argv[0][0] == NULL) break;

        if (!builtin_command(argv[i])) 
        { //quit -> exit(0), & -> ignore, other -> run
            if(bg) 
            {
                Sigprocmask(SIG_BLOCK,&p_mask,&prev_mask);
            }
            
            if((realpid = fork()) == 0) // child code
            {
                Sigprocmask(SIG_SETMASK,&prev_mask,NULL);
                Sigprocmask(SIG_BLOCK,&c_mask,&prev_mask);
            
                if (i > 0) 
                { 
                    close(fds[i - 1][1]);
                    dup2(fds[i - 1][0],0);
                    close(fds[i - 1][0]);
                    
                }
                if (i < num) 
                { 
                    close(fds[i][0]);
                    dup2(fds[i][1], 1);
                    close(fds[i][1]);
                }

                if ((execve(argv[i][0], argv[i], environ) < 0)) 
                {	//ex) /bin/ls ls -al &
                    printf("%s: Command not found.\n", argv[i][0]);
                    exit(0);
                }
                exit(0);
            }
            else 
            {
                if (i > 0) 
                {
                    close(fds[i - 1][0]);
                }
                if (i < num)
                {
                    close(fds[i][1]);
                }
                
                if (!bg)
                {    
	                int status;
                    Sigprocmask(SIG_BLOCK,&mask_all,&prev_mask);
                    add_job(realpid,argv[i][0],RUNNING_FG);
                    // printf("realpid [%d]: %d",i,realpid)

                    realpid = 0;
                    while(!realpid)
                    {
                        if(fg_check())
                        {   
                            Sigsuspend(&prev_mask);
                        }
                        else break;
                    }       
                }
                else//when there is backgrount process!
	            {
                    Sigprocmask(SIG_BLOCK,&mask_all,NULL);
                    add_job(realpid,argv[i][0],RUNNING_BG);
                    idx = find_idx(realpid);
                    if(i == num ) printf("[%d] %d\n",jobs_list[idx].id,realpid);
                }  
                Sigprocmask(SIG_SETMASK,&prev_mask,NULL); 
            }
        }
    }

    for (i = 0; i < num; i++) 
    {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    return;
}
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    // pid_t pid;           /* Process id */
    int idx;
    sigset_t p_mask,c_mask,prev_mask, mask_all;

    strcpy(buf, cmdline);
    bg = parseline(buf,argv,0);    
    // printf("bg : %d\n",bg);

    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    
    Sigfillset(&mask_all);
    Sigemptyset(&p_mask);
    Sigaddset(&p_mask,SIGCHLD);
    Sigaddset(&p_mask,SIGINT);
    Sigaddset(&p_mask,SIGTSTP);
    Sigemptyset(&c_mask);
    Sigaddset(&c_mask,SIGINT);
    Sigaddset(&c_mask,SIGTSTP);

    if (!builtin_command(argv)) 
    { //quit -> exit(0), & -> ignore, other -> run
        
        if(bg) 
        {
            Sigprocmask(SIG_BLOCK,&p_mask,&prev_mask);
        }

        if((realpid = fork()) == 0) // child code
        {
            Sigprocmask(SIG_SETMASK,&prev_mask,NULL);
            Sigprocmask(SIG_BLOCK,&c_mask,&prev_mask);
            
            if ((execve(argv[0], argv, environ) < 0)) 
            {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
	/* Parent waits for foreground job to terminate */
	    if (!bg) // fg process
        { 
            int status;
            Sigprocmask(SIG_BLOCK,&mask_all,&prev_mask);
            add_job(realpid,argv[0],RUNNING_FG);
            
            realpid = 0;
            while(!realpid)
            {
                if(fg_check())
                {   
                    Sigsuspend(&prev_mask);
                }
                else break;
            }
        }
	    else//when there is backgrount process!
	    {
            Sigprocmask(SIG_BLOCK,&mask_all,NULL);
            add_job(realpid,argv[0],RUNNING_BG);
            idx = find_idx(realpid);
            printf("[%d] %d\n",jobs_list[idx].id,realpid);
        }  
        Sigprocmask(SIG_SETMASK,&prev_mask,NULL); 
    }
    
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    sigset_t mask, prev_mask;

    if (!strcmp(argv[0], "exit") || !strcmp(argv[0],"quit")) /* quit command */
	{
        exit(0);
    }      
    
    if (!strcmp(argv[0], "cd"))
    {   
        char* path= (char *)malloc(sizeof(char)*MAXLINE);
        // "cd ~", change to the home directory.
        if( (argv[1] == NULL) || !strcmp(argv[1], "~") || !strcmp(argv[1],"$HOME") )
        {
            strcpy(path,getenv("HOME"));
        }
        else
        {
            strcpy(path,argv[1]);
        }

        if(chdir(path)) 
        {
            perror("Wrong directory!\n");
        }

        free(path);

        return 1;
    }
    // echo 아래에 넣을지 따로 뺼지 고민.
    
    if(!strcmp(argv[0], "grep")||!strcmp(argv[0], "sort")||!strcmp(argv[0], "less")||!strcmp(argv[0], "ls")||!strcmp(argv[0], "echo")||!strcmp(argv[0], "mkdir")||!strcmp(argv[0], "rmdir")||!strcmp(argv[0], "touch")||!strcmp(argv[0], "cat")||!strcmp(argv[0], "tail")||!strcmp(argv[0], "ps"))
    {
        char temp[MAXLINE] = "/bin/";
        strcat(temp,argv[0]);
        argv[0] = temp;
        return 0;
    }
    
    if(!strcmp(argv[0], "jobs"))
    {
        print_jobs();
        return 1;
    }

    if(!strcmp(argv[0], "bg"))
    {
        Sigemptyset(&mask);
        Sigaddset(&mask,SIGCHLD);
        Sigprocmask(SIG_BLOCK,&mask,&prev_mask);

        if(argv[1] == NULL || argv[1][0] != '%')
        {
            printf("Wrong index format\n");
            Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
            return 1;
        }
        
        int idx;    
        idx = atoi(&argv[1][1]);
        
        if((num_jobs > 0) && (idx >= 0) && (idx < num_jobs))
        {
            if(jobs_list[idx].state == RUNNING_BG)
            {
                printf("-bash: bg: job %d already in background\n",idx);
            }
            else
            {
                jobs_list[idx].state = RUNNING_BG;
                printf("[%d] %s\n",idx, jobs_list[idx].cmd);
                Kill(jobs_list[idx].pid,SIGCONT);
            }
        }
        else 
        {
            printf("-bash: kill: %%%d: no such job\n",idx);
        }
        Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
        return 1;
    }
    
    if(!strcmp(argv[0], "fg"))
    {
        Sigemptyset(&mask);
        Sigaddset(&mask,SIGCHLD);
        Sigprocmask(SIG_BLOCK,&mask,&prev_mask);

       if(argv[1] == NULL || argv[1][0] != '%')
        {
            printf("Wrong index format\n");
            Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
            return 1;
        }
        
        int idx;    
        idx = atoi(&argv[1][1]);
        
        if((num_jobs > 0) && (idx >= 0) && (idx < num_jobs) && ((jobs_list[idx].state == RUNNING_BG) || (jobs_list[idx].state == SUSPENDED)))
        {
            
            jobs_list[idx].state = RUNNING_FG;
            printf("[%d] %s\n",idx, jobs_list[idx].cmd);
            Kill(jobs_list[idx].pid,SIGCONT);
            
            while(!realpid){
                if(fg_check())
                {   
                    Sigsuspend(&prev_mask);
                }
                else break;
            }
        }
        else 
        {
            printf("-bash: kill: %%%d: no such job\n",idx);
        }
        Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
        return 1;
    }

    if(!strcmp(argv[0], "kill"))
    {
        Sigemptyset(&mask);
        Sigaddset(&mask,SIGCHLD);
        Sigprocmask(SIG_BLOCK,&mask,&prev_mask);

        if(argv[1] == NULL || argv[1][0] != '%')
        {
            printf("Wrong index format\n");
            Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
            return 0;
        }
        
        int idx;    
        idx = atoi(&argv[1][1]);
        
        if((num_jobs > 0) && (idx >= 0) && (idx < num_jobs))
        {
            kill_job(idx);
        }
        else 
        {
            printf("-bash: kill: %%%d: no such job\n",idx);
        }
        Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
        return 1;
    }
    
    // Sigprocmask(SIG_UNBLOCK,&mask,&prev_mask);
    
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv,int pipe_flag) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */
    int cnt = 0;


    if(!pipe_flag)
    { 
        buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    }    
    else 
    {
        if(buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = ' ';
        else strcat(buf," ");
    }

    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) 
    {
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }

    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	{
        argv[--argc] = NULL;
    }
    else if ((argv[argc-1][strlen(argv[argc-1])-1]) == '&')
    {
        bg = 1;
        argv[argc-1][strlen(argv[argc-1])-1] = '\0';
    }
    
    return bg;
}
/* $end parseline */

void del_char(char *cmdline)
{
    char *delim;
    while ((delim = strchr(cmdline, '\"'))) 
    {
	    *delim = ' ';
	    cmdline = delim + 1;
    }
    while ((delim = strchr(cmdline, '\''))) 
    {
	    *delim = ' ';
	    cmdline = delim + 1;
    }  
}
void sigint_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask, prev_one;  
    // Sio_puts("Sigint action");
    // Sio_puts("\n");
    if(!command_flag)
    {
        Sio_puts("\n");
        Sio_puts("CSE4100-SP-P2> ");
    }
    else Sio_puts("\n");
    
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK,&mask,&prev_one);
    
    for(int i=0;i<num_jobs;i++)
    {
        if(jobs_list[i].state == RUNNING_FG)
        {
            Kill(jobs_list[i].pid,SIGKILL);
            clear_job(i);
        }
    }
    Sigprocmask(SIG_SETMASK,&prev_one,NULL); 
    errno = olderrno; 
    return;
}
void sigtstp_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask, prev_one;
    // Sio_puts("Sigtstp action");
    // Sio_puts("\n");    
    if(!command_flag)
    {
        Sio_puts("\n");
        Sio_puts("CSE4100-SP-P2> ");
    }
    else Sio_puts("\n");

    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK,&mask,&prev_one);

    for(int i=0;i<num_jobs;i++)
    {
        if(jobs_list[i].state == RUNNING_FG)
        {
            Kill(jobs_list[i].pid,SIGSTOP);
            jobs_list[i].state = SUSPENDED;
        }
    }
    Sigprocmask(SIG_SETMASK,&prev_one,NULL);
    errno = olderrno; 
    return;
}
void sigchld_handler(int sig)
{
    //pid_t pid;
    int status, idx;
    sigset_t mask, prev_one;
    
    Sigfillset(&mask);
    while((realpid = waitpid(-1,&status,WNOHANG | WUNTRACED)) > 0) 
    {
        Sigprocmask(SIG_BLOCK,&mask,&prev_one);
        
        if((status == SIGPIPE)|| WIFEXITED(status)) //  || status == SIGPIPE
        {
            idx = find_idx(realpid);
            if(idx == -1) continue;
            else
            {
                clear_job(idx);
            }
                
        }
        Sigprocmask(SIG_SETMASK,&prev_one,NULL);
    }
}
void print_jobs(void)
{
    for(int i=0;i<num_jobs;i++)
    {
        if(jobs_list[i].state == RUNNING_BG)
        {
            printf("[%d]  Running\t\t %s\n",i,jobs_list[i].cmd);
        }
        else if(jobs_list[i].state == SUSPENDED)
        {
            printf("[%d]  Stopped\t\t %s\n",i,jobs_list[i].cmd);
        }
        // printf("[%d]  %d\t\t %s\n",i,jobs_list[i].state,jobs_list[i].cmd);
    }
}
void add_job(pid_t pid,char *cmd, int state)
{
    jobs_list[num_jobs].pid = pid;
    jobs_list[num_jobs].id = num_jobs;
    strcpy(jobs_list[num_jobs].cmd,cmd);
    jobs_list[num_jobs++].state = state;
}
void clear_job(int idx)
{
    jobs_list[idx].pid = 0;
    jobs_list[idx].id = -1;
    strcpy(jobs_list[idx].cmd,"");
    jobs_list[idx].state = UNDEFINED;

    for(int i=idx;i<num_jobs-1;i++)
    {
        jobs_list[i].pid = jobs_list[i+1].pid;
        jobs_list[i].id = jobs_list[i+1].id;
        strcpy(jobs_list[i].cmd,jobs_list[i+1].cmd);
        jobs_list[i].state = jobs_list[i+1].state;
    }
    num_jobs--;
}
void kill_job(int idx)
{
    
    Kill(jobs_list[idx].pid,SIGKILL);
    printf("[%d]  Terminated\t\t %s\n",idx,jobs_list[idx].cmd);
    clear_job(idx);
}
int find_idx(pid_t pid)
{
    for(int i=0;i<num_jobs;i++)
    {
        if(jobs_list[i].pid == pid)
            return i;
    }

    return -1;
}
int fg_check(void)
{
    for(int i=0;i<num_jobs;i++)
    {
        if(jobs_list[i].state == RUNNING_FG)
            return 1;
    }
    return 0;
}