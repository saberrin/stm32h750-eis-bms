// MOS_Controller.c
#include "MOS_Controller.h"
#include <math.h>

// Small spin delay for latch timing
static inline void tiny_delay(void) {
    for (volatile int i = 0; i < 200; ++i) __NOP();
}

// Generic GPIO init helper
static void GPIO_Output_Init(GPIO_TypeDef* port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

// Enable or disable all 4514 (E low = enable)
static inline void S_All_Enable(void) {
	
//		HAL_GPIO_WritePin(S1_PORT, S1_PIN, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(S2_PORT, S2_PIN, GPIO_PIN_SET);
	
    HAL_GPIO_WritePin(S1_E_PORT, S1_E_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S2_E_PORT, S2_E_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S3_E_PORT, S3_E_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S4_E_PORT, S4_E_PIN, GPIO_PIN_RESET);
}

static inline void S_All_Disable(void) {
//		HAL_GPIO_WritePin(S1_PORT, S1_PIN, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(S2_PORT, S2_PIN, GPIO_PIN_RESET);
	
    HAL_GPIO_WritePin(S1_E_PORT, S1_E_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(S2_E_PORT, S2_E_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(S3_E_PORT, S3_E_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(S4_E_PORT, S4_E_PIN, GPIO_PIN_SET);
}

// Drive shared S-A[3:0] bus
static inline void S_SetAddrBus(uint8_t ch) {
    HAL_GPIO_WritePin(S_A0_PORT, S_A0_PIN, (ch & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A1_PORT, S_A1_PIN, (ch & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A2_PORT, S_A2_PIN, (ch & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A3_PORT, S_A3_PIN, (ch & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// Pulse LE of a specific 4514 to latch the current S-A bus value
static inline void S_Latch(SwitchGroup_t g) {
    GPIO_TypeDef *LE_PORT; uint16_t LE_PIN;
    switch (g) {
        case SWITCH_GROUP_S1: LE_PORT = S1_LE_PORT; LE_PIN = S1_LE_PIN; break;
        case SWITCH_GROUP_S2: LE_PORT = S2_LE_PORT; LE_PIN = S2_LE_PIN; break;
        case SWITCH_GROUP_S3: LE_PORT = S3_LE_PORT; LE_PIN = S3_LE_PIN; break;
        default:              LE_PORT = S4_LE_PORT; LE_PIN = S4_LE_PIN; break;
    }
    HAL_GPIO_WritePin(LE_PORT, LE_PIN, GPIO_PIN_SET);
    tiny_delay();
    HAL_GPIO_WritePin(LE_PORT, LE_PIN, GPIO_PIN_RESET);
}

// Map Bk (0..52) to (4514 group, channel)
// Formula: t = 52 - Bk, group = t % 4, channel = t / 4
static inline void Map_B_to_S(uint8_t Bk, SwitchGroup_t *g_out, uint8_t *ch_out) {
    uint8_t t  = 52u - Bk;
    uint8_t gi = t & 0x3u;
    uint8_t ch = t >> 2;     // 0..13, note S1 may use 13, others up to 12
    *ch_out = ch;
    switch (gi) {
        case 0: *g_out = SWITCH_GROUP_S1; break;
        case 1: *g_out = SWITCH_GROUP_S2; break;
        case 2: *g_out = SWITCH_GROUP_S3; break;
        default:*g_out = SWITCH_GROUP_S4; break;
    }
}

// Set 74HC139 X1..X4 by rotate-right pattern per step s
// Xi(s) = (i - s) mod 4, i=0..3 -> X1..X4
static inline void X_Set_ByStep(uint8_t s) {
		/* 计算右移次数 */
    uint8_t shift = (uint8_t)((51u - s) & 0x03u);
	
    for (uint8_t i = 0; i < 4; ++i) {
        uint8_t val = (uint8_t)((i + 4 - (shift & 0x3u)) & 0x3u);
        DecoderGroup_t xg = (DecoderGroup_t)(DECODER_X1 + i);
        // Decoder_Select_2to4 keeps SEL low (active)
        Decoder_Select_2to4(xg, val);
    }
}

// Program the four adjacent B pins for step s
// This selects [B(52-s), B(51-s), B(50-s), B(49-s)]
//void SwitchWindow_Program(uint8_t s) {
////    HAL_GPIO_WritePin(S1_PORT, S1_PIN, GPIO_PIN_SET);
////    HAL_GPIO_WritePin(S2_PORT, S2_PIN, GPIO_PIN_SET);
//	
//		// Keep LE low by default
//    HAL_GPIO_WritePin(S1_LE_PORT, S1_LE_PIN, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(S2_LE_PORT, S2_LE_PIN, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(S3_LE_PORT, S3_LE_PIN, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(S4_LE_PORT, S4_LE_PIN, GPIO_PIN_RESET);

//    // Enable all 4514 before programming
//    S_All_Enable();

//    // Program four targets by latching each chip individually
//    for (uint8_t j = 0; j < 4; ++j) {
//        uint8_t Bk = (uint8_t)(52u - s - j);     // B52,B51,B50,B49 when s=0
//        SwitchGroup_t g; uint8_t ch;
//        Map_B_to_S(Bk, &g, &ch);
//        S_SetAddrBus(ch);                        // drive shared bus
//        S_Latch(g);                              // only this chip captures
//    }

//    // Set X decoders with rotate-right pattern
//    X_Set_ByStep(s);

//    // Optional short settle time for analog front-end
//    // HAL_Delay(1);
//}

void SwitchWindow_Program(uint8_t s)
{
    // ---------- 特例：s=1 或 s=52 保留，不做任何切换 ----------
    if (s == 1 || s == 52)
    {
        printf("SwitchWindow_Program: s=%d (special case, no change)\r\n", s);
        return;
    }

    // ---------- s=0：关闭所有开关 ----------
    if (s == 0)
    {
        //printf("SwitchWindow_Program: close all switches (s=0)\r\n");

        S_All_Disable();       // 禁止所有 4514 译码器
        // Disable decoder (active high)
				HAL_GPIO_WritePin(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN, GPIO_PIN_SET);       // 禁止所有 139 译码器
        return;
    }

    // ---------- s=2 ~ 51：正常窗口 ----------
    if (s >= 2 && s <= 51)
    {
        // 计算基础 B 编号：Bk = s - 2
        uint8_t startB = s - 2;

        printf("SwitchWindow_Program: s=%d → Enable B%d~B%d\r\n",
               s, startB, startB + 3);

        // 清理 LE 线
        HAL_GPIO_WritePin(S1_LE_PORT, S1_LE_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(S2_LE_PORT, S2_LE_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(S3_LE_PORT, S3_LE_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(S4_LE_PORT, S4_LE_PIN, GPIO_PIN_RESET);

        // 重新使能所有 4514
        S_All_Enable();

        // -------- 编程四个 B 通道 Bk~Bk+3 --------
        for (uint8_t j = 0; j < 4; ++j)
        {
            uint8_t Bk = startB + j;     // B0~B3, B1~B4 ...

            SwitchGroup_t g;
            uint8_t ch;
            Map_B_to_S(Bk, &g, &ch);     // 查出对应的 4514 芯片和通道

            S_SetAddrBus(ch);            // 设置地址（共享 4bit bus）
            S_Latch(g);                  // 仅对应 4514 锁存
        }

        // -------- 设置 139 译码器 --------
        X_Set_ByStep(s);

        return;
    }

    // ---------- 其他非法 s 值 ----------
    printf("ERR: SwitchWindow_Program(): invalid s=%d\r\n", s);
}




void SwitchWindow_Program_Test(uint8_t s) {
//    HAL_GPIO_WritePin(S1_PORT, S1_PIN, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(S2_PORT, S2_PIN, GPIO_PIN_SET);
	
		// Keep LE low by default
    HAL_GPIO_WritePin(S1_LE_PORT, S1_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S2_LE_PORT, S2_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S3_LE_PORT, S3_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S4_LE_PORT, S4_LE_PIN, GPIO_PIN_RESET);

    // Enable all 4514 before programming
    S_All_Enable();

	
		SwitchGroup_t g; uint8_t ch;
	
		g = SWITCH_GROUP_S1;
		ch = 0;
		Decoder_Select_4to16(g, ch);
	
		g = SWITCH_GROUP_S2;
		ch = 0;
		Decoder_Select_4to16(g, ch);
		
		g = SWITCH_GROUP_S3;
		ch = 0;
		Decoder_Select_4to16(g, ch);
		
		g = SWITCH_GROUP_S4;
		ch = 0;
		Decoder_Select_4to16(g, ch);
		
    // Set X decoders with rotate-right pattern
    X_Set_ByStep(s);

    // Optional short settle time for analog front-end
    // HAL_Delay(1);
}

// Basic decoders API (compatible with your earlier signatures)
void Decoder_Select_4to16(SwitchGroup_t group, uint8_t channel) {
    // Drive one 4514 by setting shared bus then latching only this chip
    S_SetAddrBus(channel);
    S_Latch(group);
}

void Decoder_Select_2to4(DecoderGroup_t group, uint8_t value) {
    GPIO_TypeDef *A0_PORT = NULL, *A1_PORT = NULL, *SEL_PORT = NULL;
    uint16_t A0_PIN = 0, A1_PIN = 0, SEL_PIN = 0;

    switch (group) {
        case DECODER_X1:
            A0_PORT = X1_A0_PORT; A0_PIN = X1_A0_PIN;
            A1_PORT = X1_A1_PORT; A1_PIN = X1_A1_PIN;
            SEL_PORT = HC74139_ENABLE_PORT; SEL_PIN = HC74139_ENABLE_PIN;
            break;
        case DECODER_X2:
            A0_PORT = X2_A0_PORT; A0_PIN = X2_A0_PIN;
            A1_PORT = X2_A1_PORT; A1_PIN = X2_A1_PIN;
            SEL_PORT = HC74139_ENABLE_PORT; SEL_PIN = HC74139_ENABLE_PIN;
            break;
        case DECODER_X3:
            A0_PORT = X3_A0_PORT; A0_PIN = X3_A0_PIN;
            A1_PORT = X3_A1_PORT; A1_PIN = X3_A1_PIN;
            SEL_PORT = HC74139_ENABLE_PORT; SEL_PIN = HC74139_ENABLE_PIN;
            break;
        case DECODER_X4:
            A0_PORT = X4_A0_PORT; A0_PIN = X4_A0_PIN;
            A1_PORT = X4_A1_PORT; A1_PIN = X4_A1_PIN;
            SEL_PORT = HC74139_ENABLE_PORT; SEL_PIN = HC74139_ENABLE_PIN;
            break;
        default:
            return;
    }

    // Write address bits
    HAL_GPIO_WritePin(A0_PORT, A0_PIN, (value & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(A1_PORT, A1_PIN, (value & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Enable decoder (active low)
    HAL_GPIO_WritePin(SEL_PORT, SEL_PIN, GPIO_PIN_RESET);
}

void SwitchMatrix_Init(void) {
    // Enable GPIO clocks
    __HAL_RCC_GPIOH_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOE_CLK_ENABLE();
		__HAL_RCC_GPIOD_CLK_ENABLE();
		
		// S1,S2 bus
//		GPIO_Output_Init(S1_PORT, S1_PIN);
//    GPIO_Output_Init(S2_PORT, S2_PIN);
	
    // S_A bus
    GPIO_Output_Init(S_A0_PORT, S_A0_PIN);
    GPIO_Output_Init(S_A1_PORT, S_A1_PIN);
    GPIO_Output_Init(S_A2_PORT, S_A2_PIN);
    GPIO_Output_Init(S_A3_PORT, S_A3_PIN);

    // S1..S4 LE/E
    GPIO_Output_Init(S1_LE_PORT, S1_LE_PIN);
    GPIO_Output_Init(S1_E_PORT,  S1_E_PIN);

    GPIO_Output_Init(S2_LE_PORT, S2_LE_PIN);
    GPIO_Output_Init(S2_E_PORT,  S2_E_PIN);

    GPIO_Output_Init(S3_LE_PORT, S3_LE_PIN);
    GPIO_Output_Init(S3_E_PORT,  S3_E_PIN);

    GPIO_Output_Init(S4_LE_PORT, S4_LE_PIN);
    GPIO_Output_Init(S4_E_PORT,  S4_E_PIN);

    // X1..X4 A0/A1/SEL
    GPIO_Output_Init(X1_A0_PORT, X1_A0_PIN);
    GPIO_Output_Init(X1_A1_PORT, X1_A1_PIN);
    GPIO_Output_Init(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN);

    GPIO_Output_Init(X2_A0_PORT, X2_A0_PIN);
    GPIO_Output_Init(X2_A1_PORT, X2_A1_PIN);
    GPIO_Output_Init(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN);

    GPIO_Output_Init(X3_A0_PORT, X3_A0_PIN);
    GPIO_Output_Init(X3_A1_PORT, X3_A1_PIN);
    GPIO_Output_Init(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN);

    GPIO_Output_Init(X4_A0_PORT, X4_A0_PIN);
    GPIO_Output_Init(X4_A1_PORT, X4_A1_PIN);
    GPIO_Output_Init(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN);
		
    // Default levels
    HAL_GPIO_WritePin(S_A0_PORT, S_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A1_PORT, S_A1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A2_PORT, S_A2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S_A3_PORT, S_A3_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(S1_LE_PORT, S1_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S2_LE_PORT, S2_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S3_LE_PORT, S3_LE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(S4_LE_PORT, S4_LE_PIN, GPIO_PIN_RESET);

    // Keep 4514 disabled at power-up
    S_All_Disable();

    // Disable 139 by default (SEL high)
    HAL_GPIO_WritePin(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HC74139_ENABLE_PORT, HC74139_ENABLE_PIN, GPIO_PIN_SET);

    // Default X addresses
    HAL_GPIO_WritePin(X1_A0_PORT, X1_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X1_A1_PORT, X1_A1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X2_A0_PORT, X2_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X2_A1_PORT, X2_A1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X3_A0_PORT, X3_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X3_A1_PORT, X3_A1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X4_A0_PORT, X4_A0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(X4_A1_PORT, X4_A1_PIN, GPIO_PIN_RESET);
		

}

// Self test: sweep s=0..49, read four nodes and check 3 neighbor deltas near 4V
int SelfTest_Run(float dv_min, float dv_max) {
    int fail = 0;

    for (uint8_t s = 0; s <= 49; ++s) {
        // Program the window and keep 4514 enabled
        SwitchWindow_Program(s);

        // Indices of the four nodes
        uint8_t b0 = (uint8_t)(52u - s);
        uint8_t b1 = (uint8_t)(51u - s);
        uint8_t b2 = (uint8_t)(50u - s);
        uint8_t b3 = (uint8_t)(49u - s);

        float v0 = Read_B_voltage(b0);
        float v1 = Read_B_voltage(b1);
        float v2 = Read_B_voltage(b2);
        float v3 = Read_B_voltage(b3);

        float d01 = fabsf(v0 - v1);
        float d12 = fabsf(v1 - v2);
        float d23 = fabsf(v2 - v3);

        if (d01 < dv_min || d01 > dv_max) fail++;
        if (d12 < dv_min || d12 > dv_max) fail++;
        if (d23 < dv_min || d23 > dv_max) fail++;
    }

    // Optionally disable all after test
    S_All_Disable();
    return fail == 0 ? 0 : fail;
}

// Measure impedance for one step s using outer nodes for excitation and inner nodes for sensing
float Measure_Impedance_One(uint8_t s, float excite_mA, uint32_t settle_ms) {
    if (s > 49) return -1.0f;

    // Program window and keep enabled
    SwitchWindow_Program(s);

    uint8_t Bhi = (uint8_t)(52u - s);
    uint8_t Bm1 = (uint8_t)(51u - s);
    uint8_t Bm2 = (uint8_t)(50u - s);
    uint8_t Blo = (uint8_t)(49u - s);

    // Apply excitation on outer nodes
    Apply_Excitation(Bhi, Blo, excite_mA);

    // Wait for analog front-end to settle
    if (settle_ms) HAL_Delay(settle_ms);

    // Measure differential voltage on inner nodes
    float v = Measure_Response_V(Bm1, Bm2);

    // Stop excitation
    Stop_Excitation();

    // Compute impedance Z = V / I
    float I = excite_mA / 1000.0f;
    if (I <= 0.0f) return -1.0f;
    return v / I;
}

// Full sweep s=0..49
void Measure_Impedance_Sweep(float excite_mA, uint32_t settle_ms, float Z_out[50]) {
    for (uint8_t s = 0; s <= 49; ++s) {
        Z_out[s] = Measure_Impedance_One(s, excite_mA, settle_ms);
    }
    // Optionally disable all after sweep
    S_All_Disable();
}

// Weak default hooks (replace in user code)
__weak float Read_B_voltage(uint8_t Bn) {
    // Replace with ADC read for node Bn
    (void)Bn;
    return 0.0f;
}

__weak void Apply_Excitation(uint8_t B_pos, uint8_t B_neg, float current_mA) {
    // Replace with your stimulus driver
    (void)B_pos; (void)B_neg; (void)current_mA;
}

__weak void Stop_Excitation(void) {
    // Replace with your stimulus stop
}

__weak float Measure_Response_V(uint8_t Bp, uint8_t Bn) {
    // Replace with ADC differential read between Bp and Bn
    (void)Bp; (void)Bn;
    return 0.0f;
}
