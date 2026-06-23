#include "fft_analyzer.h"
#include <math.h>
#include <stdio.h>  // ????

#define PI 3.14159265358979323846
extern uint16_t Num_period;

void Goertzel_Analyze(float * signal, uint32_t N, double target_freq, float fs, FFTOutput* result) {
    double k = (double)target_freq * N / fs;
    double omega = 2.0 * PI * k / N;
    double coeff = 2.0 * cos(omega);
    
    double q0 = 0.0, q1 = 0.0, q2 = 0.0;
    // printf("KKK %.5f  %.5f  %.5f\n", k, omega, coeff);
		uint32_t start_label = N - ( (N + Num_period -1) / Num_period )*(Num_period -1);
		// printf("start_label %u, Num %u\n", start_label,N);
    for(uint32_t i = start_label; i < N; i++) {
        q0 = coeff * q1 - q2 + signal[i];
        q2 = q1;
        q1 = q0;
//				printf("q0 = %+.6f, q1 = %+.6f, q2 = %+.6f,Signal %+.6f\n", q0, q1, q2, (double)signal[i]);
    }
    // ????(????????)
//    printf("q0 = %+.6f, q1 = %+.6f, q2 = %+.6f\n", q0, q1, q2);
    // ???float??
    result->magnitude = (float)(sqrt((q1 - q2 * cos(omega)) * (q1 - q2 * cos(omega)) + 
                        (q2 * sin(omega)) * (q2 * sin(omega))) * 2.0 / N);
    result->phase_rad = (float)atan2(q2 * sin(omega), q1 - q2 * cos(omega));
}
