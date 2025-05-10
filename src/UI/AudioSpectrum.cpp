#include "UI/AudioSpectrum.hpp"

AudioSpectrum::AudioSpectrum(const int& sampleNb, const int& fftSize)
	: SAMPLE_NB(sampleNb), FFT_SIZE(fftSize), _arrayIn(fftSize, {0, 0}), _arrayOut(fftSize, {0, 0})
{
	_config = kiss_fft_alloc(FFT_SIZE, 0, nullptr, nullptr);
	if (_config == nullptr)
	{
		Logger::log("AudioSpectrum", Error) << "KissFFT config allocation failed" << std::endl;
		exit(1);
	}

	_frequencies = new double[FFT_SIZE / 2];
	_magnitudes = new double[FFT_SIZE / 2];
	if (!_frequencies || !_magnitudes)
	{
		Logger::log("AudioSpectrum", Error) << "Allocation failed" << std::endl;
		exit(1);
	}
}

AudioSpectrum::~AudioSpectrum()
{
	kiss_fft_cleanup();
	free(_config);
	delete[] _frequencies;
	delete[] _magnitudes;
}

void AudioSpectrum::update(const Audio& audio)
{
	processAudioSpectrum(audio);
	plot(_frequencies, _magnitudes, audio.getSampleRate() / 2.0);
}

void AudioSpectrum::processAudioSpectrum(const Audio& audio)
{
	// Find in audio buffer, sound that will be played this update
	// which is not directly at writeCursor because of the latency.
	int dataStart = (int)audio.getWriteCursorPos() - (int)(audio.getFramesPerUpdate() * audio.getLatency());

	// Make sure index is in buffer
	dataStart = dataStart % (int)audio.getBufferSize();
	if (dataStart < 0) dataStart = (int)audio.getBufferSize() - abs(dataStart);

	for (int i = 0; i < SAMPLE_NB; i++)
	{
		const int bufferIndex = (dataStart + i * audio.getChannels()) % audio.getBufferSize();

		_arrayIn[i].r = hannWindowing(audio.getBuffer()[bufferIndex], i);
		if (audio.getChannels() == 2)
		{
			_arrayIn[i].r += hannWindowing(audio.getBuffer()[bufferIndex + 1], i);
			_arrayIn[i].r /= 2.0;
		}
	}

	kiss_fft(_config, _arrayIn.data(), _arrayOut.data());
	for (int i = 0; i < FFT_SIZE / 2.0; i++)
	{
		const double& real = _arrayOut[i].r;
		const double& imaginary = _arrayOut[i].i;
		_magnitudes[i] = sqrt((imaginary * imaginary) + (real * real));
	}

	for (int i = 0; i < FFT_SIZE / 2; i++)
		_frequencies[i] = (double)audio.getSampleRate() / (double)FFT_SIZE  * i;
}

double AudioSpectrum::hannWindowing(double v, unsigned int index)
{
	return v * (0.5 * (1.0 - cos(2.0 * M_PI * index / (SAMPLE_NB - 1))));
}

void AudioSpectrum::plot(const double* xAxis, const double* yAxis, const unsigned int& xMax)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	const bool windowOpened = ImGui::Begin("Audio Spectrum");
	ImGui::PopStyleVar(1);

	if (windowOpened && ImPlot::BeginPlot("Plot", ImVec2(ImGui::GetContentRegionAvail())))
	{
		ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
		ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, xMax);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 10.0); // Set Y axis go from 0 to 10
		ImPlot::PlotLine("Audio Spectrum", xAxis, yAxis, FFT_SIZE / 2);
		ImPlot::EndPlot();
	}

	ImGui::End();
}
