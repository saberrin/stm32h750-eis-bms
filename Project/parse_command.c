#include "parse_command.h"
#include "global_command.h"
#include <math.h>
#include <ctype.h>
#include "SYS_protection.h"

 uint8_t TerminalAddr = 0;          // 终端地址
 char CommandCode[16] = {0};       // 命令码缓冲区     

 
 uint8_t ParseStatus = 0;          // 解析状态：0-成功，1-格式错误，2-校验错误


// 辅助函数：去除字符串首尾空格
void trim(char* str) {
    if (str == NULL || str[0] == '\0') return;
    
    char* start = str;
    char* end = str + strlen(str) - 1;
    
    // 去除开头空格
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    // 去除结尾空格
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    
    *(end + 1) = '\0';
    
    // 移动字符串到开头
    if (start != str) {
        memmove(str, start, end - start + 2);
    }
}






// volatile安全的内存设置函数
void volatile_memset(volatile void *s, int c, size_t n) {
    volatile unsigned char *p = (volatile unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
}

// volatile安全的字符串复制函数
void volatile_strcpy(volatile char *dest, const char *src) {
    while ((*dest++ = *src++) != '\0');
}

// volatile安全的字符串复制函数（带长度限制）
void volatile_strncpy(volatile char *dest, const char *src, size_t n) {
    while (n-- && (*dest++ = *src++) != '\0');
    if (n == 0) {
        *dest = '\0';
    }
}

// volatile安全的字符串修剪函数
void volatile_trim(volatile char *str) {
    if (str == NULL || *str == '\0') return;
    
    volatile char *start = str;
    volatile char *end = str;
    
    // 找到字符串结尾
    while (*end != '\0') end++;
    end--;
    
    // 去除开头空格
    while (*start && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) {
        start++;
    }
    
    // 去除结尾空格
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        end--;
    }
    
    *(end + 1) = '\0';
    
    // 移动字符串到开头
    if (start != str) {
        volatile char *dst = str;
        volatile char *src = start;
        while ((*dst++ = *src++) != '\0');
    }
}




void ParseReceivedData(char* command)
{
    // 重置全局变量
    TerminalAddr = 0;
    volatile_memset(CommandCode, 0, sizeof(CommandCode));
    CommandParam1 = 0.0;  // 直接赋值为 0.0
    CommandParam2 = 0.0;  // 直接赋值为 0.0
    ParseStatus = 1;
    
    printf("开始解析指令: %s\r\n", command);
    
    // 1. 检查指令基本格式
    if (command == NULL || strlen(command) < 5) {
        printf("错误: 指令过短或为空\r\n");
        return;
    }
    
    // 2. 检查起始标识符
    if (command[0] != '@') {
        printf("错误: 无效的起始标识符，应为'@'\r\n");
        return;
    }
    
    // 3. 查找所有分隔符
    char* first_comma = strchr(command, ',');
    char* second_comma = first_comma ? strchr(first_comma + 1, ',') : NULL;
    char* third_comma = second_comma ? strchr(second_comma + 1, ',') : NULL;
    
    if (!first_comma) {
        printf("错误: 指令格式不完整，缺少分隔符\r\n");
        return;
    }
    
    // 4. 提取终端地址 (保持不变)
    int addr_len = first_comma - (command + 1);
    if (addr_len <= 0 || addr_len >= 16) {
        printf("错误: 终端地址长度无效\r\n");
        return;
    }
    
    char addr_str[16] = {0};
    strncpy(addr_str, command + 1, addr_len);
    addr_str[addr_len] = '\0';
    trim(addr_str);
    
    if (strlen(addr_str) > 2 && addr_str[0] == '0' && 
        (addr_str[1] == 'x' || addr_str[1] == 'X')) {
        TerminalAddr = (uint8_t)strtol(addr_str, NULL, 16);
    } else {
        TerminalAddr = (uint8_t)strtol(addr_str, NULL, 10);
    }
    
    // 5. 提取命令码 (保持不变)
    int cmd_len = 0;
    if (second_comma) {
        cmd_len = second_comma - (first_comma + 1);
    } else {
        cmd_len = strlen(command) - (first_comma - command) - 1;
    }
    
    if (cmd_len <= 0 || cmd_len >= (int)sizeof(CommandCode) - 1) {
        printf("错误: 命令码长度无效\r\n");
        return;
    }
    
    volatile_strncpy(CommandCode, first_comma + 1, cmd_len);
    CommandCode[cmd_len] = '\0';
    volatile_trim(CommandCode);
    
    // 6. 提取并转换参数1
    if (second_comma) {
        int param1_len = 0;
        char param1_str[32] = {0};  // 临时缓冲区
        
        if (third_comma) {
            param1_len = third_comma - (second_comma + 1);
        } else {
            param1_len = strlen(command) - (second_comma - command) - 1;
        }
        
        if (param1_len > 0) {
            if (param1_len >= (int)sizeof(param1_str) - 1) {
                printf("警告: 参数1过长，将被截断\r\n");
                param1_len = sizeof(param1_str) - 1;
            }
            
            // 复制参数到临时缓冲区
            strncpy(param1_str, second_comma + 1, param1_len);
            param1_str[param1_len] = '\0';
            trim(param1_str);
            
            // 验证参数格式（允许数字和小数点）
            int is_valid_param1 = 1;
            int decimal_point_count = 0;
            
            for (int i = 0; i < strlen(param1_str); i++) {
                char ch = param1_str[i];
                if (ch == '.') {
                    decimal_point_count++;
                    if (decimal_point_count > 1) {
                        is_valid_param1 = 0;  // 多个小数点
                        printf("错误: 参数1包含多个小数点\n");
                        break;
                    }
                } else if (ch < '0' || ch > '9') {
                    is_valid_param1 = 0;
                    printf("错误: 参数1包含非法字符 '%c'，只允许数字和小数点\n", ch);
                    break;
                }
            }
            
            if (!is_valid_param1) {
                ParseStatus = 2;
                CommandParam1 = 0.0;
                CommandParam2 = 0.0;
                return;
            }
            
            // 使用 strtod 转换为 double[7,8](@ref)
            char* endptr;
            double temp_value = strtod(param1_str, &endptr);
            
            // 检查转换是否成功[6](@ref)
            if (endptr == param1_str || *endptr != '\0') {
                printf("错误: 参数1转换失败\n");
                ParseStatus = 2;
                CommandParam1 = 0.0;
                return;
            }
            
            // 检查溢出[7](@ref)
            if (temp_value == HUGE_VAL || temp_value == -HUGE_VAL) {
                printf("错误: 参数1数值溢出\n");
                ParseStatus = 2;
                CommandParam1 = 0.0;
                return;
            }
            
            CommandParam1 = temp_value;
        } else {
            CommandParam1 = 0.0;  // 参数为空，默认为 0.0
        }
    } else {
        CommandParam1 = 0.0;  // 没有参数1，默认为 0.0
    }
    
    // 7. 提取并转换参数2
    if (third_comma) {
        int param2_len = strlen(command) - (third_comma - command) - 1;
        char param2_str[32] = {0};  // 临时缓冲区
        
        if (param2_len > 0) {
            if (param2_len >= (int)sizeof(param2_str) - 1) {
                printf("警告: 参数2过长，将被截断\r\n");
                param2_len = sizeof(param2_str) - 1;
            }
            
            // 复制参数到临时缓冲区
            strncpy(param2_str, third_comma + 1, param2_len);
            param2_str[param2_len] = '\0';
            trim(param2_str);
            
            // 验证参数格式（允许数字和小数点）
            int is_valid_param2 = 1;
            int decimal_point_count = 0;
            
            for (int i = 0; i < strlen(param2_str); i++) {
                char ch = param2_str[i];
                if (ch == '.') {
                    decimal_point_count++;
                    if (decimal_point_count > 1) {
                        is_valid_param2 = 0;  // 多个小数点
                        printf("错误: 参数2包含多个小数点\n");
                        break;
                    }
                } else if (ch < '0' || ch > '9') {
                    is_valid_param2 = 0;
                    printf("错误: 参数2包含非法字符 '%c'，只允许数字和小数点\n", ch);
                    break;
                }
            }
            
            if (!is_valid_param2) {
                ParseStatus = 2;
                CommandParam2 = 0.0;
                return;
            }
            
            // 使用 strtod 转换为 double[7,8](@ref)
            char* endptr;
            double temp_value = strtod(param2_str, &endptr);
            
            // 检查转换是否成功[6](@ref)
            if (endptr == param2_str || *endptr != '\0') {
                printf("错误: 参数2转换失败\n");
                ParseStatus = 2;
                CommandParam2 = 0.0;
                return;
            }
            
            // 检查溢出[7](@ref)
            if (temp_value == HUGE_VAL || temp_value == -HUGE_VAL) {
                printf("错误: 参数2数值溢出\n");
                ParseStatus = 2;
                CommandParam2 = 0.0;
                return;
            }
            
            CommandParam2 = temp_value;
        } else {
            CommandParam2 = 0.0;  // 参数为空，默认为 0.0
        }
    } else {
        CommandParam2 = 0.0;  // 没有参数2，默认为 0.0
    }
    
    // 8. 解析成功
    ParseStatus = 0;
    printf("解析成功! 地址:0x%02X, 命令:%s, 参数1:%.6f, 参数2:%.6f\r\n", 
           TerminalAddr, (const char*)CommandCode, CommandParam1, CommandParam2);
}




// 命令枚举定义
typedef enum {
    CMD_STAT,
    CMD_GETID,
    CMD_GETRT,
    CMD_GETCFG,
    CMD_GETLOG,
    CMD_GETV,
    CMD_GETI,
    CMD_GETT,
    CMD_GETE,
    CMD_GETZ,
    CMD_SET_EIS_AMP,
    CMD_SET_EIS_BIAS,
    CMD_SET_EIS_CYCLES,
    CMD_SET_EIS_FREQ_START,
    CMD_SET_EIS_FREQ_END,
    CMD_SET_EIS_FREQ_POINTS,
    CMD_SET_TEMP_HIGH_ALARM,
    CMD_SET_VOLT_CELL_HIGH,
    CMD_SET_VOLT_CELL_LOW,
    CMD_SET_CURR_CHG_ALARM,
    CMD_SET_CURR_DIS_ALARM,
    CMD_SET_CELL_COUNT,
    CMD_SET_CALIB_DATA,
    CMD_RST,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_ABORT,
		CMD_CALIBRATE,
    CMD_UNKNOWN // 未知命令标识
} CommandEnum;



// 将命令字符串映射为枚举值
CommandEnum getCommandEnum(const char* cmdCode) {
    // 使用if-else if链进行字符串比较，返回对应的枚举值
    if (strcmp(cmdCode, "STAT") == 0) return CMD_STAT;
    else if (strcmp(cmdCode, "GETID") == 0) return CMD_GETID;
    else if (strcmp(cmdCode, "GETRT") == 0) return CMD_GETRT;
    else if (strcmp(cmdCode, "GETCFG") == 0) return CMD_GETCFG;
    else if (strcmp(cmdCode, "GETLOG") == 0) return CMD_GETLOG;
    else if (strcmp(cmdCode, "GETV") == 0) return CMD_GETV;
    else if (strcmp(cmdCode, "GETI") == 0) return CMD_GETI;
    else if (strcmp(cmdCode, "GETT") == 0) return CMD_GETT;
    else if (strcmp(cmdCode, "GETE") == 0) return CMD_GETE;
    else if (strcmp(cmdCode, "GETZ") == 0) return CMD_GETZ; // 注意前缀匹配
    else if (strcmp(cmdCode, "SET_EIS_AMP") == 0) return CMD_SET_EIS_AMP;
    else if (strcmp(cmdCode, "SET_EIS_BIAS") == 0) return CMD_SET_EIS_BIAS;
    else if (strcmp(cmdCode, "SET_EIS_CYCLES") == 0) return CMD_SET_EIS_CYCLES;
    else if (strcmp(cmdCode, "SET_EIS_FREQ_START") == 0) return CMD_SET_EIS_FREQ_START;
    else if (strcmp(cmdCode, "SET_EIS_FREQ_END") == 0) return CMD_SET_EIS_FREQ_END;
    else if (strcmp(cmdCode, "SET_EIS_FREQ_POINTS") == 0) return CMD_SET_EIS_FREQ_POINTS;
    else if (strcmp(cmdCode, "SET_TEMP_HIGH_ALARM") == 0) return CMD_SET_TEMP_HIGH_ALARM;
    else if (strcmp(cmdCode, "SET_VOLT_CELL_HIGH") == 0) return CMD_SET_VOLT_CELL_HIGH;
    else if (strcmp(cmdCode, "SET_VOLT_CELL_LOW") == 0) return CMD_SET_VOLT_CELL_LOW;
    else if (strcmp(cmdCode, "SET_CURR_CHG_ALARM") == 0) return CMD_SET_CURR_CHG_ALARM;
    else if (strcmp(cmdCode, "SET_CURR_DIS_ALARM") == 0) return CMD_SET_CURR_DIS_ALARM;
    else if (strcmp(cmdCode, "SET_CELL_COUNT") == 0) return CMD_SET_CELL_COUNT;
    else if (strcmp(cmdCode, "SET_CALIB_DATA") == 0) return CMD_SET_CALIB_DATA;
    else if (strcmp(cmdCode, "RST") == 0) return CMD_RST;
    else if (strcmp(cmdCode, "PAUSE") == 0) return CMD_PAUSE;
    else if (strcmp(cmdCode, "RESUME") == 0) return CMD_RESUME;
    else if (strcmp(cmdCode, "ABORT") == 0) return CMD_ABORT;
		else if (strcmp(cmdCode, "CALIBRATE") == 0) return CMD_CALIBRATE;
		
    else return CMD_UNKNOWN; // 未知命令
}




//-----------------------------------------------------------------
// CommandExecStatus ExecuteCommand(char* input)
//-----------------------------------------------------------------
CommandExecStatus ExecuteCommand(char* input)
{
    // 1. 解析指令
    ParseReceivedData(input);
    
    if (ParseStatus != 0) {
        printf("指令解析失败\r\n");
        return CMD_EXEC_INVALID_PARAM;
    }
    
    printf("执行命令: 地址=0x%02X, 指令=%s, 参数1=%.6f,参数2=%.6f\r\n", 
           TerminalAddr, CommandCode, CommandParam1,CommandParam2);
    
    // 2. 检查地址匹配（修正您的逻辑错误）
    if ((TerminalAddr != 0x00) && ( TerminalAddr !=QG_ID)) {
        printf("地址不匹配: 本地=0x%02X, 目标=0x%02X\r\n", QG_ID, TerminalAddr);
        return CMD_EXEC_ADDR_MISMATCH;
    }
		
		
		CommandEnum cmdEnum = getCommandEnum(CommandCode);
		
		//使用switch-case根据枚举值执行不同分支[1,3,4]
   switch (cmdEnum) {
        case CMD_STAT:
            printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取系统状态的具体逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETID:
            printf("执行GETID命令\n");
            // 在这里直接编写获取设备ID的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETRT:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取实时数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETCFG:
            printf("执行GETCFG命令\n");
            // 在这里直接编写获取配置信息的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETLOG:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取日志信息的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETV:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取电压数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETI:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取电流数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETT:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);            
				      printf(">0x%02X,GETT_OK,%.2f\r\n", QG_ID, read_ambient_temperature());
				
			
				// 在这里直接编写获取温度数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETE:
             g_current_state=SYS_EIS_SWEEP ;
				printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写能量测量或EIS测量的启动逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETZ:
         g_current_state=SYS_EIS_SINGLE ;
				printf("执行GETS_XX命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取特定传感器数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_AMP:
            printf("执行SET_EIS_AMP命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS振幅的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_BIAS:
            printf("执行SET_EIS_BIAS命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS偏置的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_CYCLES:
            printf("执行SET_EIS_CYCLES命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS周期数的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_FREQ_START:
            printf("执行SET_EIS_FREQ_START命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS起始频率的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_FREQ_END:
            printf("执行SET_EIS_FREQ_END命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS结束频率的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_EIS_FREQ_POINTS:
            printf("执行SET_EIS_FREQ_POINTS命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置EIS频率点数的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_TEMP_HIGH_ALARM:
            printf("执行SET_TEMP_HIGH_ALARM命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置高温报警阈值的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_VOLT_CELL_HIGH:
            printf("执行SET_VOLT_CELL_HIGH命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置单体电压高报警的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_VOLT_CELL_LOW:
            printf("执行SET_VOLT_CELL_LOW命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置单体电压低报警的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_CURR_CHG_ALARM:
            printf("执行SET_CURR_CHG_ALARM命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置充电电流报警的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_CURR_DIS_ALARM:
            printf("执行SET_CURR_DIS_ALARM命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置放电电流报警的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_CELL_COUNT:
            printf("执行SET_CELL_COUNT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置电芯数量的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_SET_CALIB_DATA:
            printf("执行SET_CALIB_DATA命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置校准数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_RST:
            printf("执行RST命令\n");
            // 在这里直接编写系统复位的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_PAUSE:
            printf("执行PAUSE命令\n");
            // 在这里直接编写暂停当前操作的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_RESUME:
            printf("执行RESUME命令\n");
            // 在这里直接编写恢复操作的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_ABORT:
            printf("执行ABORT命令\n");
            // 在这里直接编写中止当前操作的逻辑
            return CMD_EXEC_SUCCESS;

				case CMD_CALIBRATE:
		            printf("执行CALIBRATE命令\n");
            // 在这里直接编写中止当前操作的逻辑
				    g_current_state=SYS_CALIBRATION ;
            return CMD_EXEC_SUCCESS;		
        case CMD_UNKNOWN:
        default:
            printf("未知命令: %s\n", CommandCode);
            return CMD_EXEC_UNKNOWN_CMD;
    }
		
						

}








