#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>

#include "err_codes.h"
//defines
#define __CMND_LINE_INPUT_SIZE_MAX 520
#define __MAX_NUM_PARAMS_SIZE 200
#define __SHELL "/bin/sh"
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
error input(char * str , size_t* sz , size_t maxSz,FILE * inp_file); //takes input from user of max size maxSz
void error_msg(error err); //display error messages with error codes in stderr
error execute(char *str , size_t sz);
error executeFore(char *str[] );
error executeBack(char *str[] );
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
            while(1){
                input(main_input_line,&main_input_line_size,512,fp);
                printf("shell>");
                puts(main_input_line);
                main_input_line_size = clearSpaces(main_input_line,main_input_line_size);
                if(main_input_line_size == 0){
                    continue;
                }
                if(!strcmp(main_input_line,"exit")){
                    terminate();
                    break;
                }
                execute(main_input_line,main_input_line_size);
            }
        }
    }else if(argc == 1){
        puts("----------------- welcome to shell program ---------------------");
        while(1){
            printf("shell>");
            error err_cd = input(main_input_line,&main_input_line_size,512,stdin);
            main_input_line_size = strlen(main_input_line);
            if(err_cd.error_code != __ERR_CODES_SUCCESS){
                error_msg(err_cd);
                continue;
            }
            main_input_line_size = clearSpaces(main_input_line,main_input_line_size);
            if(main_input_line_size == 0){
                    continue;
            }
            if(!strcmp(main_input_line,"exit")){
                terminate();
                break;
            }
            execute(main_input_line,main_input_line_size);
        }
    }else{
        //error
        terminate();
    }
    return 0;
}
//function implementation
 void initialize(){
    history_file = fopen("history.txt","a");
}
void terminate(){
    puts("----------------- program exited ---------------------");
}
error input(char * buff , size_t *sz , size_t maxSz,FILE * inp_file){
    error err_temp = {__ERR_CODES_FAIL};
    if(buff == NULL){
        return err_temp;
    }
    char chr = 1;sz = 0;int i =0;
    chr = getchar();
    while(chr != '\n' && chr != EOF){
        buff[i++] = chr,sz++;
        if(i == maxSz+1){
            while(chr != '\n' && chr != EOF){
                chr = fgetc(inp_file);
            }
            err_temp.error_code = __ERR_CODES_BIG_SIZE_INPUT;
            return err_temp;
        }
        buff[i] = 0;
        chr = fgetc(inp_file);
    }
    err_temp.error_code = __ERR_CODES_SUCCESS;
    return err_temp;
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
    default: fputs("error happened",stderr);break;
    }
}

error execute(char * str , size_t sz){
    char * token = strtok(str , " ");
    char *params[__MAX_NUM_PARAMS_SIZE];
    int indx = 2;
    params[0] = __SHELL;params[1] = "-c";
    params[indx] = token;
    while(token != NULL){
        token = strtok(NULL , " ");
        params[++indx];
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
      /* This is the child process.  Execute the shell command. */
      status = execv (str[0] , str);
      _exit (EXIT_FAILURE);
    }
    else if (pid < 0)
        /* The fork failed.  Report failure.  */
        status = -1;
    else
        /* This is the parent process.  Wait for the child to complete.  */
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
