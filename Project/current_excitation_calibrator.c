#include "current_excitation_calibrator.h"

// 全局变量定义
CurrentCalibration_t CurrentCalib = {0};
UART_HandleTypeDef * CalibratorUartHandle = NULL;

// --- 模块内部全局变量和配置 ---
#define CALIB_VOLTAGE_START    (0.0f)   // 起始电压 (V)
#define CALIB_VOLTAGE_END      (2.0f)   // 结束电压 (V)
#define CALIB_POINTS           (10)     // 校准数据点数量
#define CALIB_VOLTAGE_STEP     ((CALIB_VOLTAGE_END - CALIB_VOLTAGE_START) / (CALIB_POINTS - 1)) // 电压步进

static float voltage_list[CALIB_POINTS]; // 存储电压点
static float current_list[CALIB_POINTS]; // 存储电流点
static uint8_t current_calib_point_index = 0; // 当前校准点索引

// 校准状态机
typedef enum {
  CALIB_STATE_IDLE,                   // 空闲状态，等待开始命令
  CALIB_STATE_WAITING_FOR_MEASUREMENT // 已设置电压，等待用户测量并上报电流
} CalibrationState_t;

static CalibrationState_t calib_state = CALIB_STATE_IDLE;

// 静态函数声明
static void StartCalibration(void);
static void ProcessCurrentMeasurement(float measured_current);
static void CalculateLinearFit(void);
static void TrimCommand(char *cmd);

// --- 函数实现 ---

// 修剪命令字符串末尾的空白字符
static void TrimCommand(char *cmd) {
    if (cmd == NULL) return;
    int len = strlen(cmd);
    while (len > 0 && isspace((unsigned char)cmd[len-1])) {
        cmd[--len] = '\0';
    }
}

// 计算线性拟合参数 (a, b)
static void CalculateLinearFit(void) {
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
    uint8_t n = CALIB_POINTS;

    for (int i = 0; i < n; i++) {
        sumX += voltage_list[i];
        sumY += current_list[i];
        sumXY += voltage_list[i] * current_list[i];
        sumX2 += voltage_list[i] * voltage_list[i];
    }

    float denom = (n * sumX2 - sumX * sumX);
    if (denom == 0) {
        printf("Error: Linear fit calculation failed (denominator is zero).\r\n");
        return;
    }

    CurrentCalib.gain_a = (n * sumXY - sumX * sumY) / denom;
    CurrentCalib.offset_b = (sumY - CurrentCalib.gain_a * sumX) / n;
    CurrentCalib.is_calibrated = 1;

    printf("\r\n=== Calibration Complete ===\r\n");
    printf("Linear Formula: I = (%.6f) * V + (%.6f)\r\n", CurrentCalib.gain_a, CurrentCalib.offset_b);
    printf("Calibration parameters saved.\r\n");
}

// 开始校准流程
static void StartCalibration(void) {
    printf("\r\n=== Starting Current Calibration ===\r\n");
    printf("Pattern: %d points, from %.1fV to %.1fV.\r\n", CALIB_POINTS, CALIB_VOLTAGE_START, CALIB_VOLTAGE_END);
    printf("Please measure current after each voltage setting and send 'CURRENT <value>'.\r\n");
     DAC1_Init_Constant();
    current_calib_point_index = 0;
    calib_state = CALIB_STATE_WAITING_FOR_MEASUREMENT;

    // 设置第一个电压点
    float first_voltage = CALIB_VOLTAGE_START;
    DAC1_Set_Voltage(first_voltage); // 调用您的DAC设置函数
    voltage_list[current_calib_point_index] = first_voltage;

    printf("Point %d/%d: DAC set to %.3fV. Measure current and send: CURRENT <measured_value>\r\n",
           current_calib_point_index + 1, CALIB_POINTS, first_voltage);
}

// 处理上报的电流值
static void ProcessCurrentMeasurement(float measured_current) {
    if (current_calib_point_index >= CALIB_POINTS) {
        printf("Error: Data point index out of range.\r\n");
        calib_state = CALIB_STATE_IDLE;
        return;
    }

    // 记录电流值
    current_list[current_calib_point_index] = measured_current;
    printf("  Recorded: V=%.3fV, I=%.6fA\r\n", voltage_list[current_calib_point_index], measured_current);

    current_calib_point_index++;

    // 检查是否所有点都已采集完毕
    if (current_calib_point_index >= CALIB_POINTS) {
        printf("\r\nAll %d data points collected. Calculating linear fit...\r\n", CALIB_POINTS);
        CalculateLinearFit();
        calib_state = CALIB_STATE_IDLE;
        printf("Calibration finished.\r\n");
    } else {
        // 设置下一个电压点
        float next_voltage = CALIB_VOLTAGE_START + (current_calib_point_index * CALIB_VOLTAGE_STEP);
        DAC1_Set_Voltage(next_voltage); // 调用您的DAC设置函数
        voltage_list[current_calib_point_index] = next_voltage;

        printf("Point %d/%d: DAC set to %.3fV. Measure current and send: CURRENT <measured_value>\r\n",
               current_calib_point_index + 1, CALIB_POINTS, next_voltage);
    }
}

// --- 公共函数 ---

// 初始化校准器
void CurrentAcquisitionCalibrator_Init(UART_HandleTypeDef * huart) {
    CalibratorUartHandle = huart;
    CurrentCalib.gain_a = 1.0f;
    CurrentCalib.offset_b = 0.0f;
    CurrentCalib.is_calibrated = 0;
    calib_state = CALIB_STATE_IDLE;

    char welcome_msg[] = "\r\nCurrent Calibrator Ready.\r\n"
                         "Send 'STARTCAL' to begin automatic calibration.\r\n";
    HAL_UART_Transmit(CalibratorUartHandle, (uint8_t*)welcome_msg, strlen(welcome_msg), HAL_MAX_DELAY);
}

// 处理从串口接收到的命令
void CurrentCalibrator_ProcessCommand(char * rx_command) {
    TrimCommand(rx_command);

    // 1. 开始校准命令
    if (strcmp(rx_command, "STARTCAL") == 0) {
        if (calib_state == CALIB_STATE_IDLE) {
            StartCalibration();
        } else {
            printf("Error: Calibration is already in progress.\r\n");
        }
        return;
    }

    // 2. 电流值上报命令
    if (strncmp(rx_command, "CURRENT", 7) == 0) {
        float measured_current;
        if (sscanf(rx_command, "CURRENT %f", &measured_current) == 1) {
            if (calib_state == CALIB_STATE_WAITING_FOR_MEASUREMENT) {
                ProcessCurrentMeasurement(measured_current);
            } else {
                printf("Error: Not in calibration measurement mode. Send 'STARTCAL' first.\r\n");
            }
        } else {
            printf("Error: Invalid CURRENT command. Usage: CURRENT <value>  (e.g., CURRENT 0.0052)\r\n");
        }
        return;
    }

    printf("Error: Unknown command '%s'. Supported: STARTCAL, CURRENT <value>\r\n", rx_command);
}
