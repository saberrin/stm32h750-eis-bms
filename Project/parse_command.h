//#include "system.h"
//#include <string.h>
//#include <stdlib.h>   // ? atof() ? atoi() ????

//#define CMD_IDLE 0x0000
//#define CMD_PRINT_VOLT 0x0001
//#define CMD_PRINT_TEM 0x0002
//#define CMD_PRINT_UIT 0x0003
//#define CMD_VERSION 0x0004
//#define CMD_COPYRIGHT 0x0005
//#define CMD_START 0x0006
//#define CMD_DEVELOPER 0x0007
//#define CMD_ABORT 0x0008
//#define CMD_HELLO 0x0009
//#define CMD_PAUSE 0x000A
//#define CMD_RESUME 0x000B
//#define CMD_LOOP_TEST 0x000C
//#define CMD_HELP 0x000D
//#define CMD_UNKNOWN 0xFFFF

//void print_QG_system_parameter(void);
//void QingGeng_help(void);


#ifndef PARSE_COMMAND_H
#define  PARSE_COMMAND_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


// ÃüÁîÖ´ÐÐ×´Ì¬Ã¶¾Ù
typedef enum {
    CMD_EXEC_SUCCESS = 0,
    CMD_EXEC_UNKNOWN_CMD,
    CMD_EXEC_INVALID_PARAM,
    CMD_EXEC_HARDWARE_ERROR,
	  CMD_EXEC_ADDR_MISMATCH
} CommandExecStatus;



// º¯ÊýÉùÃ÷
CommandExecStatus ExecuteCommand(char* input);
void RegisterAllCommands(void);


void ParseReceivedData( char* input);


void Cmd_SendResp(const char *cmd, const char *cmd_stat, const char *data);






#endif // COMMAND_HANDLER_H

























