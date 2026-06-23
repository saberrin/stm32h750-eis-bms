//#ifndef __FFT_ANALYZER_H
//#define __FFT_ANALYZER_H

//#include <stdint.h>
//#include <complex.h>

//typedef struct {
//    float magnitude;
//    float phase_rad;
//} FFTOutput;

//void Goertzel_Analyze(float * signal, uint32_t N, double target_freq, float fs, FFTOutput* result);

//#endif
#ifndef __FFT_ANALYZER_H
#define __FFT_ANALYZER_H

#include <stdint.h>
#include <complex.h>

typedef struct {
    float magnitude;
    float phase_rad;
} FFTOutput;

void Goertzel_Analyze(float * signal, uint32_t N, double target_freq, float fs, FFTOutput* result);

#endif
