#pragma once

#include "inc.hpp"
#include "kiss_fft.h"
#include "Logger.hpp"

class AudioSpectrum {
private:
	kiss_fft_cfg _config;
	const int SAMPLE_NB;
	const int FFT_SIZE;
	std::vector<kiss_fft_cpx> _arrayIn;
	std::vector<kiss_fft_cpx> _arrayOut;

	// Arrays used to plot values
	double* _frequencies;
	double* _magnitudes;

	void processAudioSpectrum(const AudioData& audio);
	double hannWindowing(double v, unsigned int index);
	void plot(const double* xAxis, const double* yAxis, const unsigned int& xMax);

public:
	AudioSpectrum(const int& sampleNb, const int& fftSize);
	~AudioSpectrum();
	void update(const AudioData& audio);
};
