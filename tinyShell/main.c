#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "err_codes.h"
//defines
#define __CMND_LINE_INPUT_SIZE_MAX 520
#define __MAX_NUM_PARAMS_SIZE 200
#define __SHELL "/bin/sh"
#define __MAX_SHELL_LINE_SIZE 513
#define __HISTORY_FILE_PATH "/tmp/history.txt"
// global vars
typedef struct {
    enum errcodes_type_enum error_code;
}error;
FILE * history_file;
char main_input_line[__CMND_LINE_INPUT_SIZE_MAX];
size_t main_input_line_size;
// functions prototypes
void terminate(); //terminates the program
void initialize();
size_t clearSpaces(char * str,int sz); //clears extra spaces in str
void error_msg(error err); //display error messages with error codes in stderr
void outputFile(char * fileName);

error execute(char *str , size_t sz);
error executeFore(char *str[] );
error executeBack(char *str[] );

error executeCD(char *str[]);
error executeHistory(char *str[]);

error chckdir(char * dir);
//main
int main(int argc , char ** argv)
{
    initialize();
    if(argc == 2){
        error err_tmp = {__ERR_CODES_SUCCESS};
        FILE *fp = fopen(argv[1],"r");
        if(fp == NULL){
            err_tmp.error_code = __ERR_CODES_INVALID_FILE;
            error_msg(err_tmp);
        }else{
            while(!fgets(main_input_line,__MAX_SHELL_LINE_SIZE,stdin)){
                printf("shell>");
                puts(main_input_line);
                main_input_line_size = clearSpaces(main_input_line,main_input_line_size);
                if(main_input_line_size < 2){
                    continue;
                }
               err_cd = execute(main_input_line,main_input_line_size);
               if(err_cd.error_code != __ERR_CODES_SUCCESS){
                    error_msg(err_cd);
                }
            }
        }
    }else if(argc == 1){
        error err_cd = {__ERR_CODES_SUCCESS};
        while(1){
            printf("shell>");
            fgets(main_input_line,__MAX_SHELL_LINE_SIZE,stdin);
            main_input_line_size = strlen(main_input_line);
            fputs(main_input_line,history_file);
            main_input_line_size = clearSpaces(main_input_line,main_input_line_size);
            if(main_input_line_size < 2){
                    continue;
            }
            err_cd = execute(main_input_line,main_input_line_size);
            if(err_cd.error_code != __ERR_CODES_SUCCESS){
                error_msg(err_cd);
            }
        }
    }else{
        //error
        terminate();
    }
    return 0;
}
//function implementation
 void initialize(){
         puts("----------------- welcome to shell program ---------------------");
    history_file = fopen(__HISTORY_FILE_PATH,"a");
    if(history_file == NULL){
        history_file = fopen(__HISTORY_FILE_PATH,"wb");
        if(history_file == NULL){
            error err_tmp ={__ERR_CODES_HISTORY_FILE_OPEN};
            error_msg(err_tmp);
            terminate();
        }
    }
}
void terminate(){
    puts("----------------- program exited ---------------------");
    close(history_file);
    exit(0);
}
void outputFile(char *fileName){
    FILE *fp_tmp = fopen(fileName,"r");
    if(fp_tmp == NULL)
        return;
    char input_line[__MAX_SHELL_LINE_SIZE];
    while(fgets(input_line,__MAX_SHELL_LINE_SIZE,fp_tmp) != NULL){
        if(strcmp(input_line,"\n"))
        printf("%s",input_line);
    }
}
size_t clearSpaces(char *str,int sz){
    int i = 0,main_pntr = 0;
    char *tmp = (char *)malloc(sz+1);
    strcpy(tmp,str);
    for(i = 0 ; i < sz ; i++){
        if(main_pntr > 0){
            if(tmp[i] == ' '){
                if(str[main_pntr-1] != ' '){
                    str[main_pntr++] = tmp[i];
                }
            }else{
                str[main_pntr++] = tmp[i];
            }
        }else{
            if(tmp[i] != ' '){
                str[main_pntr++] = tmp[i];
            }
        }
        str[main_pntr] = 0;
    }
    if(main_pntr > 0 && str[main_pntr-1] == ' '){
        str[--main_pntr] = 0;
    }
    return main_pntr;
}


void error_msg(error err){
    switch(err.error_code){
    case __ERR_CODES_SUCCESS:
        break;
    case __ERR_CODES_FAIL:
        fputs("unknown error hanppend",stderr);break;
    case __ERR_CODES_BIG_SIZE_INPUT:
        fputs("very big input size",stderr);break;
    case __ERR_CODES_INVALID_FILE:
        fputs("file can't be opened (opened by another program or invalid path)",stderr);break;
    case __ERR_CODES_WRONG_NUM_PARAMS:
        fputs("Wrong number of parameters please enter right number of parameters",stderr);break;
    case __ERR_CODES_HISTORY_FILE_OPEN:
        fputs("cannot open history file",stderr);break;
    case __ERR_CODES_NOSUCHDIR:
        fputs("no such directory",stderr);break;
    default: fputs("error happened",stderr);break;
    }
}

error execute(char * str , size_t sz){
    char * token = strtok(str , " \n");
    char *params[__MAX_NUM_PARAMS_SIZE];
    int indx = 0;//2;
//    params[0] = __SHELL;//params[1] = "-c";
    params[indx] = token;
    while(token != NULL){
        token = strtok(NULL , " \n");
        params[++indx] = token;
    }
    if(!strcmp(params[0] ,"exit")){
        terminate();
    }else if (!strcmp(params[0] , "cd")){
        return executeCD(params);
    }else if(!strcmp (params[0] , "history")){
        return executeHistory(params);
    }
    if(str[sz-1] == '&'){
        return executeBack(params);
    }else{
        return executeFore(params);
    }
}

error executeFore(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    int status;
    pid_t pid;
    pid = fork ();
    if (pid == 0)
    {
       execvp (str[0] , str);
      _exit (EXIT_FAILURE);
    }
    else if (pid < 0)
        status = -1;
    else
        if (waitpid (pid, &status, 0) != pid)
            status = -1;
    if(status == -1)
        status = __ERR_CODES_FAIL;
    err_tmp.error_code = status;
    return err_tmp;
}
error executeBack(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    return err_tmp;
}
error executeHistory(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    if(str[1] != NULL){
        err_tmp.error_code = __ERR_CODES_WRONG_NUM_PARAMS;
    }else
        outputFile(__HISTORY_FILE_PATH);
    return err_tmp;
}
error executeCD(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
     if(str[2] != NULL){
        if(str[1] == NULL){ chdir("/home/");char* tmp_direc[200];
        getwd(tmp_direc);puts(tmp_direc);return err_tmp;}
        else
            err_tmp.error_code = __ERR_CODES_WRONG_NUM_PARAMS;
    }else{
        char* tmp_direc[200];
        getwd(tmp_direc);
        strcat(tmp_direc,"/");
        strcat(tmp_direc,str[1]);
        err_tmp = chckdir(tmp_direc);
        if(err_tmp.error_code == __ERR_CODES_SUCCESS)
            chdir(str[1]);
            getwd(tmp_direc);
            puts(tmp_direc);
        }
    return err_tmp;
}
error chckdir(char * dir){
    error err_tmp = {__ERR_CODES_SUCCESS};
    struct stat s;
    int err_num = stat(dir,&s);
    if(-1 == err_num) {
            err_tmp.error_code = __ERR_CODES_NOSUCHDIR;

    } else {
        if(S_ISDIR(s.st_mode)) {
            //do nothing
        } else {
            err_tmp.error_code = __ERR_CODES_NOSUCHDIR;
        }
    }
    return err_tmp;
}
