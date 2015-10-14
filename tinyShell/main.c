//general
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
//linux specials :D
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
//user files
#include "err_codes.h"
//defines
#define __CMND_LINE_INPUT_SIZE_MAX 520  // size of input line size that takes the user input or file line
#define __MAX_NUM_PARAMS_SIZE 200 //number of parameters per command
#define __SHELL "/bin/sh" //shell path
#define __MAX_SHELL_LINE_SIZE 513 //max size of a command
#define __HISTORY_FILE_PATH "/tmp/history.txt" // history file permenant path
#define __LOG_FILE_PATH "/tmp/log_tinyshell_mohamed.txt" //log file permenant path
#define __WORK_DIREC_SIZE 200 //current working directorry allocation size
#define __REALLOC_VEC_SIZE 1 //size added to vector when reallocating
#define __USRNAME_SIZE 100 // user name allocation size
// global vars
typedef struct {
    enum errcodes_type_enum error_code;
}error; // struct for passing errors between functions

FILE * history_file,* logFilePntr; //history file and log file

char main_input_line[__CMND_LINE_INPUT_SIZE_MAX]; // one user or batch file line
size_t main_input_line_size;

int __MAX_VEC_SIZE = 2; //current vector capacity
char ** LHS_VEC;char ** RHS_VEC;// vector for varaiables and its values
int curr_vec_ind=0; //variables vector size

char * pathEnvVar; //the "PATH" environment variable
char  userNameStr[__USRNAME_SIZE]; // machine username
// functions prototypes

void terminate(); //terminates the program
void initialize();//initializes program
void getPathEnvVar();//sets pathEnvVar to the value of "PATH"
void getUserName();//set userNameStr to the current username
void error_msg(error err); //display error messages with error codes in stderr

void outputFile(char * filePath);//outputs file to the terminal
FILE * openFile(FILE * fp , char * path);//returns a pointer to the file provided in path variable


void dirty_work(char * str);//do the dirty work while dealing with the line
error execute(char *str , size_t sz);//takes the line and checks if it is special command
                                    //(cd , exit , history) and choose to execute it in
                                    // forground or backgorund
error executeFore(char *str[] );//execute command in foreground
error executeBack(char *str[] );//execute command in background
error executeDummy(char * params[]);//takes parameters and executes the command by searching for its path in PATH var

error executeCD(char *str[]);// special handling for 'cd' command
error executeHistory(char *str[]);// special handling for 'history' command
error chdirHome();//special handling for 'cd' shortcut to home

error checkInput(char * str,int sz);//checks input for '"'
error check_cmnd_is_path(char * params[]);// checks that command is a path itself
error tryThisPath(char * path , char * params[]);//tries the command in the path path.
error chckdir(char * dir);//checks if directory exists
size_t clearSpaces(char * str,int sz); //clears extra spaces in str

boolean isVarDeclaration(char * str);//checks if current line is variable declaration
error executeDeclaration(char * str[]);//executes declaration by saving varaible and its value

void setParams(char * str , size_t sz , char * params[]);//a strtok like function that handle quotation
void modifyParams(char *params[]);//searches parameters for shell or env. variables and replace them


void handleAmpersand(char * str[]);// removes '&' from the command
void handleProcessTerminated(int sig);//log that process was terminated
void writeStartSession();//write a spearator in both history and log file for every new session

int findLHS(char * str);//searches variable in our variable vector
void initializeVec();//initializes variable vector

//main
int main(int argc , char ** argv)
{
    error err_tmp = {__ERR_CODES_SUCCESS};
    initialize();
    if(argc == 2){//if the program is run with one argument
        FILE *fp = fopen(argv[1],"r"); // open the file specified in the argument
        if(fp == NULL){//if not found make an error and exit
            err_tmp.error_code = __ERR_CODES_INVALID_FILE;
            error_msg(err_tmp);
            terminate();
        }else{//if the file is found read it line by line
            while(fgets(main_input_line,__MAX_SHELL_LINE_SIZE,fp)!= NULL){// if not EOF
                printf(KMAG "shell" KWHT "@" KRED "%s" KWHT ">",userNameStr);
                puts(main_input_line);//print the line in batch mode
                dirty_work(main_input_line);//start working on the line
            }
        }
    }else if(argc == 1){
        while(1){
            printf(KCYN "shell" KWHT "@" KRED "%s" KWHT ">",userNameStr);
            char* fgets_return_var = fgets(main_input_line,__MAX_SHELL_LINE_SIZE,stdin);//read user command
            if(fgets_return_var == NULL){// if the user typed ctrl+D (EOF)
                terminate();
            }
            dirty_work(main_input_line);//start working on the line
        }
    }else{
        //error argument are more than expected
        err_tmp.error_code = __ERR_CODES_WRONG_NUM_ARGS;
        error_msg(err_tmp);
        terminate();
    }
    //never reached
    return 0;
}
//function implementation
 void initialize(){
    puts("----------------- welcome to shell program ---------------------");
    history_file = openFile(history_file,__HISTORY_FILE_PATH);
    logFilePntr = openFile(logFilePntr,__LOG_FILE_PATH);
    getPathEnvVar();
    getUserName();
    initializeVec();
    signal(SIGCHLD, handleProcessTerminated) //initialize child processes termination alert
    writeStartSession();
}
void initializeVec(){
    LHS_VEC = (char **)realloc(LHS_VEC,__MAX_VEC_SIZE*sizeof(char*));//allocate inital size for the vector
    RHS_VEC = (char **)realloc(RHS_VEC,__MAX_VEC_SIZE*sizeof(char*));
}
void writeStartSession(){
    fputs(" -*-*-*-*-*-*-*-*-*-*-*-*  new session started -*-*-*-*-*-*-*-*-*-*-*-*\n" , history_file);
    fputs(" -*-*-*-*-*-*-*-*-*-*-*-*  new session started -*-*-*-*-*-*-*-*-*-*-*-*\n" , logFilePntr);
}
void getPathEnvVar(){
    pathEnvVar = getenv("PATH");//query system for var "PATH"
    if(pathEnvVar == NULL){//couldn't get $PATH
        error err_tmp = {__ERR_CODES_PATH_VAR_FAILED};
        error_msg(err_tmp);
        terminate();
    }
}
FILE * openFile(FILE * fp , char * path){
    fp = fopen(path,"a");//read file and append to it
    if(fp == NULL){
        fp = fopen(path,"wb");
        if(fp == NULL){
            error err_tmp ={__ERR_CODES_HISTORY_LOG_FILE_OPEN};
            error_msg(err_tmp);
            terminate();
        }
    }
    return fp;
}
void terminate(){
    puts("\n----------------- program exited ---------------------");
    fclose(history_file);fclose(logFilePntr);//close pointers
    free(LHS_VEC);free(RHS_VEC);
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
error checkInput(char * str,int sz){//checks that input has even number of ' " '
    int quotCnt = 0,i=0;
    for(i = 0 ; i < sz  ; i++){
        if(str[i] == '"'){
            quotCnt++;
        }
    }
    error err_tmp = {__ERR_CODES_SUCCESS};
    if(quotCnt&1){// if number is od output error
        err_tmp.error_code = __ERR_CODE_INVALID_QUOTE_FORMAT;
    }
    return err_tmp;
}
void dirty_work(char * str){
    error err_cd = {__ERR_CODES_SUCCESS};
    main_input_line_size = strlen(main_input_line);
    fputs(main_input_line,history_file);//put line in the history file
    err_cd = checkInput(main_input_line,main_input_line_size);
    if(err_cd.error_code != __ERR_CODES_SUCCESS){
        error_msg(err_cd);
        return;
    }
    main_input_line_size = clearSpaces(main_input_line,main_input_line_size);
    if(main_input_line_size < 2){
        return;
    }
    if(main_input_line[0] == '#'){
        return;
    }
    err_cd = execute(main_input_line,main_input_line_size);
    if(err_cd.error_code != __ERR_CODES_SUCCESS){
        error_msg(err_cd);
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
        if(tmp[i] == '\"'){
            while(tmp[++i] != '\"' && i < sz){
                str[main_pntr++] = tmp[i];
            }
            str[main_pntr++] = tmp[i];
        }
        str[main_pntr] = 0;
    }
    if(main_pntr > 0 && str[main_pntr-1] == ' '){
        str[--main_pntr] = 0;
    }
    free(tmp);
    return main_pntr;
}


void error_msg(error err){
    switch(err.error_code){
    case __ERR_CODES_SUCCESS:
        break;
    case __ERR_CODES_FAIL:
        fputs("unknown error hanppend",stderr);break;
    case __ERR_CODES_FAIL_EXEC:
        fputs("wrong command",stderr);break;
    case __ERR_CODES_BIG_SIZE_INPUT:
        fputs("very big input size",stderr);break;
    case __ERR_CODES_INVALID_FILE:
        fputs("file can't be opened (opened by another program or invalid path)",stderr);break;
    case __ERR_CODES_WRONG_NUM_PARAMS:
        fputs("Wrong number of parameters please enter right number of parameters",stderr);break;
    case __ERR_CODES_HISTORY_LOG_FILE_OPEN:
        fputs("cannot open history or log file",stderr);break;
    case __ERR_CODES_NOSUCHDIR:
        fputs("no such directory",stderr);break;
    case __ERR_CODES_PATH_VAR_FAILED:
        fputs("couldn't get path variable",stderr);break;
    case __ERR_CODES_DUMY_EXEC_FILE_NOTFOUND:
        fputs("couldn't find this command",stderr);break;
    case __ERR_CODE_INVALID_QUOTE_FORMAT:
        fputs("invalid format: number of quotations is not even",stderr);break;
    case __ERR_CODES_WRONG_NUM_ARGS:
        fputs("wrong number of arguments to the program expected tinyShell [filename]",stderr);break;
    case __ERR_CODES_WRONG_DECLARATION_SYNTAX:
        fputs("invalid syntax for declaration ( no spaces in declaration and one '=' only )",stderr);break;
    default: fputs("error happened",stderr);break;
    }
    fputs("\n",stderr);
}

error execute(char * str , size_t sz){
//    char * token = strtok(str , " \n");
    char *params[__MAX_NUM_PARAMS_SIZE];
 /*   int indx = 0;//2;
//    params[0] = __SHELL;//params[1] = "-c";
    params[indx] = token;
    while(token != NULL){
        token = strtok(NULL , " \n");
        params[++indx] = token;
    }*/
    setParams(str,sz,params);
    modifyParams(params);
    if(!strcmp(params[0] ,"exit")){
        terminate();
    }else if (!strcmp(params[0] , "cd")){
        return executeCD(params);
    }else if(!strcmp (params[0] , "history")){
        return executeHistory(params);
    }else if(isVarDeclaration(params[0]) == __TRUE ){
        return executeDeclaration(params);
    }
    if(str[sz-2] == '&'){
        return executeBack(params);
    }else{
        return executeFore(params);
    }
}
void modifyParams(char *params[]){
    int indx = 0;
    char * token ;
    while(params[indx] != NULL){
        if(params[indx][0] == '$'){
            token = params[indx]+1;
            int vecInd = findLHS(token);
            if( vecInd!= -1){
                params[indx] = RHS_VEC[vecInd];
            }else{
                token = getenv(token);
                if(token != NULL){
                    params[indx] = token;
                }else
                    params[indx] = "";
            }
        }
        indx++;
    }
}
int findLHS(char * str){
    int i = 0;
    for(i = 0 ; i < curr_vec_ind ;i++){
        if(!strcmp(LHS_VEC[i],str)){
            return i;
        }
    }
    return -1;
}
void setParams(char * str , size_t sz , char * params[]){
    int i = 0,flag = 0,curParamInd = 0 , curStrInd = 0;
    char * newChr = malloc(__MAX_SHELL_LINE_SIZE), * allocChar;
    for(i = 0 ; i < sz ; i++){
        if(str[i] == ' ' && !flag){
            allocChar = (char *)malloc(curStrInd+1);
            strcpy(allocChar,newChr);
            params[curParamInd++] = allocChar;
            curStrInd = 0;
            continue;
        }else if(str[i] == '\"'){
            flag = !flag;
            continue;
        }else if(str[i] == '\n'){
            continue;
        }
        newChr[curStrInd++] = str[i];
        newChr[curStrInd] = 0;
    }
    if(curStrInd > 0){
        allocChar = (char *) malloc(curStrInd+1);
        strcpy(allocChar,newChr);
        params[curParamInd++] = allocChar;
    }
    params[curParamInd] = NULL;
    free(allocChar);free(newChr);
}
error executeFore(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    int status = 0;
    pid_t pid;
    pid = fork ();
    if (pid == 0)
    {
       err_tmp = executeDummy (str);
       if(err_tmp.error_code != __ERR_CODES_SUCCESS){
            error_msg(err_tmp);
       }
      _exit (EXIT_SUCCESS);
    }
    else if (pid < 0)
        err_tmp.error_code = __ERR_CODES_FAIL;
    else
        if (waitpid (pid, &status, 0) != pid){
            err_tmp.error_code = __ERR_CODES_FAIL;
        }
    return err_tmp;
}
error executeBack(char *str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    handleAmpersand(str);
    pid_t pid;
    pid = fork ();
    if (pid == 0) //child process
    {
       err_tmp = executeDummy (str);
       if(err_tmp.error_code != __ERR_CODES_SUCCESS){
            error_msg(err_tmp);
       }
      _exit (EXIT_SUCCESS);
    }
    else if (pid < 0)
        err_tmp.error_code = __ERR_CODES_FAIL;
    //else parent process do nothing

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
        if(str[1] == NULL || str[1][0] == '~'){return chdirHome();}
        else
            err_tmp.error_code = __ERR_CODES_WRONG_NUM_PARAMS;
    }else if(str[1] == NULL || str[1][0] == '~') {
        return chdirHome();
    }else if (str[1][0] == '/' ){
        chdir(str[1]);
    }else{
        char tmp_direc[__WORK_DIREC_SIZE];
        getcwd(tmp_direc,__WORK_DIREC_SIZE);
        strcat(tmp_direc,"/");
        strcat(tmp_direc,str[1]);
        err_tmp = chckdir(tmp_direc);
        if(err_tmp.error_code == __ERR_CODES_SUCCESS)
            chdir(str[1]);
            getcwd(tmp_direc,__WORK_DIREC_SIZE);
            puts(tmp_direc);
        }
    return err_tmp;
}
error executeDeclaration(char * str[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    if(str[1] != NULL){
        err_tmp.error_code = __ERR_CODES_WRONG_DECLARATION_SYNTAX;
        return err_tmp;
    }
    char* LHS = strtok(str[0],"=");
    char* RHS = strtok(NULL , "=\n");
    if(strtok(NULL , "=\n") != NULL){
        err_tmp.error_code = __ERR_CODES_WRONG_DECLARATION_SYNTAX;
        return err_tmp;
    }
    LHS_VEC[curr_vec_ind] = LHS;RHS_VEC[curr_vec_ind++] = RHS;
    if(curr_vec_ind == __MAX_VEC_SIZE){
        __MAX_VEC_SIZE += __REALLOC_VEC_SIZE;
        LHS_VEC = (char **)realloc(LHS_VEC,__MAX_VEC_SIZE*sizeof(char*));
        RHS_VEC = (char **)realloc(RHS_VEC,__MAX_VEC_SIZE*sizeof(char*));
    }
    if(LHS_VEC == NULL || RHS_VEC == NULL){
        err_tmp.error_code = __ERR_CODES_FAIL;
        error_msg(err_tmp);
        terminate();
    }
    puts(RHS_VEC[curr_vec_ind-1]);
    return err_tmp;
}
error chdirHome(){
    error err_tmp = {__ERR_CODES_SUCCESS};
    //chdir("/home");
    char  tmpHomePath[20] = "/home/";
    strcat(tmpHomePath,userNameStr);
    chdir(tmpHomePath);
    char tmp_direc[__WORK_DIREC_SIZE];
        getcwd(tmp_direc,__WORK_DIREC_SIZE);
        puts(tmp_direc);
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
error executeDummy(char * params[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    err_tmp = check_cmnd_is_path(params);
    if(err_tmp.error_code == __ERR_CODES_SUCCESS){
        return err_tmp;
    }
    char * token = strtok(pathEnvVar,":");
    while(token != NULL){
        err_tmp = tryThisPath(token , params);
        if(err_tmp.error_code == __ERR_CODES_SUCCESS){
            break;
        }else if (err_tmp.error_code != __ERR_CODES_DUMY_EXEC_FILE_NOTFOUND){
            return err_tmp;
        }
        token = strtok(NULL,":");
    }
    return err_tmp;
}
void handleAmpersand(char * str[]){
    int ind=0,i = 0;
    while(str[++ind] != NULL);
    if(!strcmp(str[ind-1] , "&")) str[ind-1] = NULL;
    else{
        for( i = strlen(str[ind-1]) -1 ; i > -1  ; i--){
            if(str[ind-1] [i] == '&'){
                str[ind-1] [i] = 0;
                break;
            }
            str[ind-1] [i] = 0;
        }
    }
}
error check_cmnd_is_path(char * params[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    if( access( params[0], F_OK ) != -1 ) {
        // file exists
        int execRetVal = execv(params[0],params);
        if(execRetVal == -1){
            err_tmp.error_code = __ERR_CODES_FAIL_EXEC;
            printf("%d",errno);
            return err_tmp;
        }
    } else {
        // file doesn't exist
        err_tmp.error_code = __ERR_CODES_DUMY_EXEC_FILE_NOTFOUND;
    }
    return err_tmp;
}
error tryThisPath(char * path , char * params[]){
    error err_tmp = {__ERR_CODES_SUCCESS};
    char tempPath[__WORK_DIREC_SIZE];
    strcat(tempPath,path);
    strcat(tempPath,"/");
    strcat(tempPath, params[0]);
    if( access( tempPath, F_OK ) != -1 ) {
        // file exists
        int execRetVal = execv(tempPath,params);
        if(execRetVal == -1){
            err_tmp.error_code = __ERR_CODES_FAIL;
            printf("%d",errno);
            return err_tmp;
        }
    } else {
        // file doesn't exist
        err_tmp.error_code = __ERR_CODES_DUMY_EXEC_FILE_NOTFOUND;
    }
    return err_tmp;
}
void handleProcessTerminated(int sig){
    fprintf(logFilePntr,"CHILD PROCESS %d TERMINATED \n",sig);
}
boolean isVarDeclaration(char * str){
    int i =0;
    for(i = 0 ; i < strlen(str) ; i++){
        if(str[i] == '='){
            return __TRUE;
        }
    }
    return __FALSE;
}
void getUserName(){
    getlogin_r(userNameStr, __USRNAME_SIZE);
}
