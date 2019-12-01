#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

char * MYPATH;
char ** mp_arr;
char cwd[2000];
int num_of_tokens;
int num_of_paths;
int debug = 1;

void init(char ** input)//initializes dynamic memory
{
    int j = 0;
    while(j<50)
    {
        input[j] = (char*)calloc(1,257*sizeof(char));
        mp_arr[j] = (char*)calloc(1,257*sizeof(char));
        j++;
    }
}

int tokenize_input(char ** input)//tokenizes command line input to dynamic memory
{
    char name[2000];
    char * token ;
    printf("%s$ ",cwd);
    fgets(name,2000,stdin);
    if(strcmp("\n\0",name)==0){return -1;}
    token = strtok(name, " \0");
    num_of_tokens = 0;
    while (token!=NULL)
    {
        strcpy(input[num_of_tokens], token);
        num_of_tokens++;
        token = strtok(NULL, " \0");
    }

    if(strcmp(input[num_of_tokens-1],"\n")==0)
    {
        num_of_tokens--;
        return 0;
    }
    input[num_of_tokens-1] = strcat(strtok(input[num_of_tokens-1], "\n"),"\0");
    if(debug)
    {
        printf("number of tokens : %d\n", num_of_tokens);
        printf("You entered ");
        int i = 0;
        while (i<num_of_tokens)
        {
            printf("%s ",input[i]);
            i++;
        }
        printf("\n");
    }
    return 1;
}

void cleanup(char ** input)//frees dynamic memory
{
    int i = 0;
    while (i<50)
    {
        free(input[i]);
        free(mp_arr[i]);
        i++;
    }
    free(input);
    free(MYPATH);
    free(mp_arr);
}

int retval_check(int retval, char** input)//for exit
{
    if (retval == -1)
    {
        sleep(2);
        printf("bye\n");
        cleanup(input);
        return -1;
    }
    return 0;
}

int path_parse()//parses the MYPATH into something usable
{
    char* token;
    token = strtok(MYPATH,"#");
    if(token == NULL){return 0;}
    num_of_paths = 0;
    while (token!=NULL)
    {
        strcpy(mp_arr[num_of_paths], token);
        num_of_paths++;
        token = strtok(NULL, "#\0");
    }
    if(debug)
    {
        int i = 0;
        printf("available paths are: \n");
        while(i<num_of_paths)
        {
            printf("%s\n",mp_arr[i]);
            i++;
        }
    }
    return 0;
}

int directory_finder(char * com, char * exec)//finds if a command exe is in a directory
{
    struct stat file_info;
    char * word = (char*)calloc(1,257*sizeof(char));
    int i = 0;
    int status = -1;
    while (i < num_of_paths)
    {

        strcpy(word, mp_arr[i]);
        if(word[strlen(word)-1]=='#')
        {
            word[strlen(word)-1]= '\0';
        }
        strcat(word,"/");
        strcat(word,com);
        if(debug){printf("checking for: %s\n",word);}

        status = lstat(word, &file_info);
        if(debug){if(status == 0){printf("found!\n");}else{printf("not found!\n");}}

        if(status == 0)
        {
            strcpy(exec,word);
            free(word);
            return 0;
        }

        i++;
    }

    free(word);
    return -1;

}

void handle_cd(char * word)
{
    if (num_of_tokens<2)
    {
        chdir(getenv("HOME"));
        getcwd(cwd,2000);
    }
    else
    {
        chdir(word);
        getcwd(cwd,2000);
    }
}

void handle_exec_no_pipe(char ** input, int waitbool)
{
    if(waitbool == 1){num_of_tokens--;}
    char* exec =  (char*)calloc(1,1000*sizeof(char));
    int found = directory_finder(input[0],exec);
    if(found == 0)
    {

        pid_t pid;
        pid = fork();
        int status;
        if(pid>0)//parent
        {
            if(waitbool == 0)
            {
                waitpid(pid, &status, 0);
                free(exec);
            }
            else if(waitbool ==1)
            {
                printf("[running background process \"%s\"]\n",input[0]);
//                waitpid(pid, &status , WNOHANG);
//                printf("here! child terminated\n");
                free(exec);
            }

        }
        else //child
        {
            char ** new_input = (char**)calloc(num_of_tokens+1,sizeof(char*));
            int i = 0;
            while(i<num_of_tokens)
            {
                new_input[i] = (char*)calloc(1,(strlen(input[i])+1)*sizeof(char));
                strcpy(new_input[i],input[i]);
                i++;
            }
            if(debug){printf("exec = %s\n",exec);}
            execv(exec,new_input);
            perror( "execv() failed" );
            exit(0);
        }
    }
    else
    {
        free(exec);
        fprintf(stderr,"ERROR: command \"%s\" not found\n",input[0]);
    }
}

void check_for_children()
{
    int status = 0;
    int retval = 0;

    while(1)
    {
        retval = waitpid(-1, &status , WNOHANG);
        if(retval >0)
        {
            printf("[process %d terminated with exit status %d]\n",retval,status);
        }
        else{return;}
    }

}

int parse(char ** input)//parses user input to run commands
{
    int i = 0;
    int and_count = 0;
    int pipe_count = 0;
    int pipe_token = 0;

    while (i<num_of_tokens) //checks
    {
        if (strcmp("exit",input[i])==0){return -1;}
        else if (strcmp("|",input[i])==0){pipe_count++; pipe_token = i;}
        else if (strcmp("&",input[i])==0){and_count++;}
        i++;
    }
    if(debug){printf("& %d, | %d\n",and_count,pipe_count);}//
    if(pipe_count == 0 && and_count == 0) //handles easy commands
    {
        if(strcmp(input[0],"cd")==0)
        {
            handle_cd(input[1]);
        }
        else
        {
            handle_exec_no_pipe(input,0);
        }
    }
    else if(pipe_count == 0 && and_count == 1) //background processing
    {
        if(strcmp(input[0],"cd")==0)
        {
            handle_cd(input[1]);
        }
        else
        {
            handle_exec_no_pipe(input,1);
        }
    }
    else if(pipe_count == 1)
    {
        if(and_count == 1){num_of_tokens--;}
        int k = 0;
        int p[2];
        pipe(p);

        char ** input_left  = (char**)calloc(pipe_token,sizeof(char*));
        char ** input_right = (char**)calloc(num_of_tokens+ 2 - pipe_token,sizeof(char*));
        while(k<pipe_token)
        {
            input_left[k] = (char*)calloc(1,(strlen(input[k])+1)*sizeof(char));
            strcpy(input_left[k],input[k]);
            k++;
        }
        k++;// skip pipe token
        while(k<num_of_tokens)
        {
            input_right[k-pipe_token-1] = (char*)calloc(1,(strlen(input[k])+1)*sizeof(char));
            strcpy(input_right[k-pipe_token-1],input[k]);
            k++;
        }
        char* exec1 =  (char*)calloc(1,1000*sizeof(char));
        char* exec2 =  (char*)calloc(1,1000*sizeof(char));
        int found1 = directory_finder(input_left[0],exec1);
        int found2 = directory_finder(input_right[0],exec2);
        if((found1 == 0) & (found2 == 0 ))
        {
            pid_t pidl, pidr;

            pidl = fork();
            if(pidl == 0)
            {
                close(p[0]);//read
                dup2(p[1],1);
//                printf("yes\n");
                execv(exec1,input_left);
                exit(0);
            }
            else
            {
                int status1;
                int status2;
                if(and_count == 0){waitpid(pidl, &status1 , 0);}
                if(and_count == 1){
                    printf("[running background process \"%s\"]\n",input_left[0]);
                   // waitpid(pidl, &status1 , WNOHANG);
                }

                pidr = fork();
                if (pidr == 0)
                {
                    close(p[1]);//write
                    dup2(p[0],0);
                    execv(exec2,input_right);
                    exit(0);
                }
                else
                {
                    if(and_count == 0)
                    {
                        close(p[1]);
                        close(p[0]);
                        waitpid(pidr, &status2 , 0);
                    }
                    if(and_count == 1)
                    {
                        close(p[1]);
                        close(p[0]);
                        printf("[running background process \"%s\"]\n",input_right[0]);

//                        waitpid(pidr, &status2 , WNOHANG);
                    }

                    int h = 0;
                    while(h < pipe_token)
                    {
                        free(input_left[h]);
                        h++;
                    }
                    h=0;
                    while(h < (num_of_tokens - pipe_token+1))
                    {
                        free(input_right[h]);
                        h++;
                    }
                    free(input_left);
                    free(input_right);
                    free(exec1);
                    free(exec2);
                }
            }
        }
        else
        {
            if(found1 == -1)
            {
                fprintf(stderr,"ERROR: command \"%s\" not found\n", input_left[0]);
            }
            if(found2 == -1)
            {
                fprintf(stderr,"ERROR: command \"%s\" not found\n", input_right[0]);
            }
            int h = 0;
            while(h < pipe_token)
            {
                free(input_left[h]);
                h++;
            }
            h=0;
            while(h < (num_of_tokens - pipe_token+1))
            {
                free(input_right[h]);
                h++;
            }
            free(input_left);
            free(input_right);
            free(exec1);
            free(exec2);
        }
    }
    else
    {
        printf("ERROR: undefined input\n");
    }

    return 0;
}

int main(void)
{
    setvbuf( stdout, NULL, _IONBF, 0 );
    MYPATH = (char*)calloc(1,257*sizeof(char));
    char* quick = getenv("MYPATH");

    if (quick==NULL){quick = "/bin#";}
    strcpy(MYPATH,quick);
//    printf("Mypath set to %s\n", MYPATH);
    getcwd(cwd,2000);
    char** input;
    input = (char**)calloc(50,sizeof(char*));
    mp_arr = (char**)calloc(50,sizeof(char*));
    int retval = 0;
    int ex = 0;
    init(input);
    path_parse();

    while(retval==0)
    {
        check_for_children();
        tokenize_input(input);
        check_for_children();
        retval = parse(input);
        check_for_children();
        ex = retval_check(retval,input);
        check_for_children();
        if(ex == -1){return 0;}
    }
    return 0;
}
