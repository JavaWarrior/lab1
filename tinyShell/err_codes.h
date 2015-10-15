#ifndef ERR_CODES_H_INCLUDED
#define ERR_CODES_H_INCLUDED

//errors typdef
enum errcodes_type_enum {__ERR_CODES_SUCCESS,
__ERR_CODES_FAIL,
__ERR_CODES_FAIL_EXEC,
__ERR_CODES_BIG_SIZE_INPUT,
__ERR_CODES_INVALID_FILE,
__ERR_CODES_WRONG_NUM_PARAMS,
__ERR_CODES_HISTORY_LOG_FILE_OPEN,
__ERR_CODES_NOSUCHDIR,
__ERR_CODES_PATH_VAR_FAILED,
__ERR_CODES_DUMY_EXEC_FILE_NOTFOUND,
__ERR_CODE_INVALID_QUOTE_FORMAT,
__ERR_CODES_WRONG_NUM_ARGS,
__ERR_CODES_WRONG_DECLARATION_SYNTAX};

//boolean typdef
typedef enum __boolean{__TRUE,__FALSE}boolean;

// output colors defines
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#endif // ERR_CODES_H_INCLUDED
