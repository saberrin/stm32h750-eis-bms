#include "parse_command.h"
#include "global_command.h"
#include <math.h>
#include <ctype.h>
#include "SYS_protection.h"
#include "identity_flash.h"
#include "runtime_flash.h"
#include "fdcan.h"
#include "EIS_Measure.h"

 uint8_t TerminalAddr = 0;          // 终端地址
 char CommandCode[32] = {0};       // 命令码缓冲区     

 
 uint8_t ParseStatus = 0;          // 解析状态：0-成功，1-格式错误，2-校验错误

// 系统状态字符串映射
const char *SystemStateStr[] = {
    "READY",
    "FAULT",
    "WARNING",
    "EIS_SWEEP",
    "EIS_SINGLE",
    "GITT_MEASURE",
    "CALIBRATION"
};

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
		CMD_IDRST,
		CMD_RTRST,
    CMD_ERASE_ALL_FLASH, 
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
		else if (strcmp(cmdCode, "IDRST") == 0) return CMD_IDRST;
		else if (strcmp(cmdCode, "RTRST") == 0) return CMD_RTRST;
		else if (strcmp(cmdCode, "ERASEALL") == 0) return CMD_ERASE_ALL_FLASH;

    else return CMD_UNKNOWN; // 未知命令
}


//所有返回的命令调用该函数统一输出格式
void Cmd_SendResp(const char *cmd, const char *cmd_stat, const char *data)
{
    char line_buffer[128];

    snprintf(line_buffer, sizeof(line_buffer),
             ">0x%02X, %s, %.6f, %.6f, %s, %s<\r\n",
             QG_ID,
             cmd,
						 CommandParam1,
						 CommandParam2,
						 cmd_stat,
             data);

    /* UART */
    printf("%s", line_buffer);

    /* CAN */
    FDCAN1_Send_String((uint8_t *)line_buffer);
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
					{
							const char *state_str = "";
							state_str = SystemStateStr[g_current_state];
							Cmd_SendResp("STAT", "CMD_OK", state_str);
							return CMD_EXEC_SUCCESS;
					}

				/* ===================== 信息读取 ===================== */
				case CMD_GETID:
						IdFlash_Dump(&g_id_in_flash);
						return CMD_EXEC_SUCCESS;

				case CMD_GETRT:
						RuntimeFlash_Dump(&g_runtime_in_flash);
						return CMD_EXEC_SUCCESS;

				case CMD_GETCFG:
						ConfigFlash_Dump(&current_cfg_in_flash);
						return CMD_EXEC_SUCCESS;

        case CMD_GETLOG:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取日志信息的逻辑
            return CMD_EXEC_SUCCESS;
				
				/* ===================== 传感量 ===================== */
        case CMD_GETV:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取电压数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETI:
             printf("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写获取电流数据的逻辑
            return CMD_EXEC_SUCCESS;

        case CMD_GETT:        
				     {
								Send_Line("执行STAT命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
								char data[32];
								float t = read_ambient_temperature();
								snprintf(data, sizeof(data), "%.2f", t);
								Cmd_SendResp("GETT", "CMD_OK", data);
								return CMD_EXEC_SUCCESS;
							}			

				/* ===================== EIS 测量 ===================== */
        case CMD_GETE:
             g_current_state=SYS_EIS_SWEEP ;
						 printf("执行GETE命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            return CMD_EXEC_SUCCESS;

        case CMD_GETZ:
						g_current_state=SYS_EIS_SINGLE ;
						printf("执行GETZ命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            return CMD_EXEC_SUCCESS;
				

				/* ===================== EIS 参数设置 ===================== */
        case CMD_SET_EIS_AMP:
						{
								QG_ACVoltPP = (float)CommandParam1;
								ConfigFlash_SaveIfChanged();

								QG_ConfigFlash_t cfg;
								if (ConfigFlash_Read(&cfg) && (cfg.ac_volt_pp == QG_ACVoltPP))
										Cmd_SendResp("SET_EIS_AMP", "CMD_OK", "");
								else
										Cmd_SendResp("SET_EIS_AMP", "FLASH_ERR", "");

								return CMD_EXEC_SUCCESS;
						}

        case CMD_SET_EIS_BIAS:
						{
								QG_DCVolt = (float)CommandParam1;
								ConfigFlash_SaveIfChanged();

								QG_ConfigFlash_t cfg;
								if (ConfigFlash_Read(&cfg) && (cfg.dc_volt == QG_DCVolt))
										Cmd_SendResp("SET_EIS_BIAS", "CMD_OK", "");
								else
										Cmd_SendResp("SET_EIS_BIAS", "FLASH_ERR", "");

								return CMD_EXEC_SUCCESS;
						}

				case CMD_SET_EIS_FREQ_START:
				{
						QG_EIS_FREQ_START = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.sweep_start_freq == QG_EIS_FREQ_START))
								Cmd_SendResp("SET_EIS_FREQ_START", "CMD_OK", "");
						else
								Cmd_SendResp("SET_EIS_FREQ_START", "FLASH_ERR", "");


						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_EIS_FREQ_END:
				{
						QG_EIS_FREQ_END = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.sweep_stop_freq == QG_EIS_FREQ_END))
								Cmd_SendResp("SET_EIS_FREQ_END", "CMD_OK", "");
						else
								Cmd_SendResp("SET_EIS_FREQ_END", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_EIS_FREQ_POINTS:
				{
						QG_EIS_FREQ_POINTS = (uint32_t)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.sweep_points == QG_EIS_FREQ_POINTS))
								Cmd_SendResp("SET_EIS_FREQ_POINTS", "CMD_OK", "");
						else
								Cmd_SendResp("SET_EIS_FREQ_POINTS", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}
				
				/* ===================== 阈值配置 ===================== */
        case CMD_SET_TEMP_HIGH_ALARM:
				{
						QG_TEMP_HIGH_ALARM = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.temp_high_alarm == QG_TEMP_HIGH_ALARM))
								Cmd_SendResp("SET_TEMP_HIGH_ALARM", "CMD_OK", "");
						else
								Cmd_SendResp("SET_TEMP_HIGH_ALARM", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_VOLT_CELL_HIGH:
				{
						QG_VOLT_CELL_HIGH = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.volt_cell_high == QG_VOLT_CELL_HIGH))
								Cmd_SendResp("SET_VOLT_CELL_HIGH", "CMD_OK", "");
						else
								Cmd_SendResp("SET_VOLT_CELL_HIGH", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_VOLT_CELL_LOW:
				{
						QG_VOLT_CELL_LOW = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.volt_cell_low == QG_VOLT_CELL_LOW))
								Cmd_SendResp("SET_VOLT_CELL_LOW", "CMD_OK", "");
						else
								Cmd_SendResp("SET_VOLT_CELL_LOW", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_CURR_CHG_ALARM:
				{
						QG_CURR_CHG_ALARM = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.curr_chg_alarm == QG_CURR_CHG_ALARM))
								Cmd_SendResp("SET_CURR_CHG_ALARM", "CMD_OK", "");
						else
								Cmd_SendResp("SET_CURR_CHG_ALARM", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_CURR_DIS_ALARM:
				{
						QG_CURR_DIS_ALARM = (float)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.curr_dis_alarm == QG_CURR_DIS_ALARM))
								Cmd_SendResp("SET_CURR_DIS_ALARM", "CMD_OK", "");
						else
								Cmd_SendResp("SET_CURR_DIS_ALARM", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}

				case CMD_SET_CELL_COUNT:
				{
						QG_CELL_COUNT = (uint32_t)CommandParam1;
						ConfigFlash_SaveIfChanged();

						QG_ConfigFlash_t cfg;
						if (ConfigFlash_Read(&cfg) && (cfg.cell_count == QG_CELL_COUNT))
								Cmd_SendResp("SET_CELL_COUNT", "CMD_OK", "");
						else
								Cmd_SendResp("SET_CELL_COUNT", "FLASH_ERR", "");

						return CMD_EXEC_SUCCESS;
				}


        case CMD_SET_CALIB_DATA:
            printf("执行SET_CALIB_DATA命令，参数1：%.6f，参数2：%.6f\r\n", CommandParam1,CommandParam2);
            // 在这里直接编写设置校准数据的逻辑
            return CMD_EXEC_SUCCESS;
				
				/* ===================== 系统控制 ===================== */
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

				case CMD_IDRST:
						printf("Reset Identity to defaults...\r\n");
						IdFlash_ResetToDefaultAndSave();
						IdFlash_Dump(&g_id_in_flash);
						return CMD_EXEC_SUCCESS;

				case CMD_RTRST:
						printf("Reset Runtime to defaults...\r\n");
						RuntimeFlash_ResetToDefaultAndSave();
						RuntimeFlash_Dump(&g_runtime_in_flash);
						return CMD_EXEC_SUCCESS;

				case CMD_ERASE_ALL_FLASH:
						printf("执行ERASEALL命令：擦除 Config/Calib/ID/Runtime 四个参数区！\r\n");
						Flash_EraseAllParamZones();
						break;


        case CMD_UNKNOWN:
        default:
            printf("未知命令: %s\n", CommandCode);
            return CMD_EXEC_UNKNOWN_CMD;
    }
		
						
    return CMD_EXEC_UNKNOWN_CMD;
}







