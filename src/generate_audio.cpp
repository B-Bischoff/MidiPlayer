#include "inc.hpp"
#include "audio_backend.hpp"
#include "AudioBackend/Components/Components.hpp"
#include "AudioBackend/Instrument.hpp"

#include <fstream>

void applyLowPassFilter(double& sample)
{
	static double lowPassState = 0.0;
	double lowPassAlpha = 0.01;
	lowPassState = lowPassAlpha * sample + (1.0 - lowPassAlpha) * lowPassState;
	sample = lowPassState;
}

// [TODO] think to group everything (or not?) in a class/struct
void generateAudio(AudioData& audio, std::vector<Instrument>& instruments, std::vector<MidiInfo>& keyPressed, double& time)
{
	static int TEST = 0;
	double fractionalPart = audio.getFramesPerUpdate() - (int)audio.getFramesPerUpdate();
	int complementaryFrame = 0;
	bool writeOneMoreFrame = false;
	if (fractionalPart != 0.0)
	{
		complementaryFrame = std::ceil(1.0 / fractionalPart);
		writeOneMoreFrame = TEST % complementaryFrame == 0;
	}
	TEST++;

	static bool init = true;
	static std::vector<int16_t> audio_buffer;
	static int counter = 0;
	if (init)
	{
		init = false;
		const char* mp3_filename = "/home/brice/Downloads/Do Re Mi Fa So La Ti Do - Piano.mp3";

		std::ifstream file(mp3_filename, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << mp3_filename << std::endl;
			exit(1);
		}

		// Read the file into a buffer
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		std::vector<uint8_t> mp3_data(size);
		if (!file.read(reinterpret_cast<char*>(mp3_data.data()), size)) {
			std::cerr << "Failed to read file: " << mp3_filename << std::endl;
			exit(1);
		}
		file.close();

		// Decode MP3 data
		mp3dec_ex_t mp3_decoder;
		if (mp3dec_ex_open_buf(&mp3_decoder, mp3_data.data(), mp3_data.size(), MP3D_SEEK_TO_SAMPLE) != 0) {
			std::cerr << "Failed to initialize MP3 decoder" << std::endl;
			exit(1);
		}

		Logger::log("HERE") << std::endl;

		// Allocate buffer for decoded audio
		audio_buffer.reserve(mp3_decoder.samples);
		size_t samples_decoded = mp3dec_ex_read(&mp3_decoder, audio_buffer.data(), mp3_decoder.samples);
		if (samples_decoded == 0) {
			std::cerr << "Failed to decode MP3 data" << std::endl;
			mp3dec_ex_close(&mp3_decoder);
			exit(1);
		}

		// Print information about the audio
		std::cout << "Decoded " << samples_decoded << " samples from " << mp3_filename << std::endl;
		std::cout << "Sample rate: " << mp3_decoder.info.hz << " Hz" << std::endl;
		std::cout << "Channels: " << mp3_decoder.info.channels << std::endl;
		Logger::log("test") << audio_buffer.size() << std::endl;

		// Cleanup
		mp3dec_ex_close(&mp3_decoder);
	}
	//std::cout << writeOneMoreFrame << std::endl;
	//for (int i = 0; i < audio.sampleRate / audio.targetFPS + (int)writeOneMoreFrame; i++)

	for (int i = 0; i < (int)audio.getFramesPerUpdate() + writeOneMoreFrame + audio.samplesToAdjust; i++)
	{
		//std::cout << " >>> " << audio.sampleRate / audio.targetFPS + (i % 3 == 0) << std::endl;
		double tmp;
		//double t = programElapsedTime.count() + (1.0f / (double)audio.sampleRate * (double)(i));
		double value = 0.0;

		value = 0.0;
		for (Instrument& instrument : instruments)
			value += instrument.process(keyPressed) * 1.0;

		//if (writeOneMoreFrame && i == audio.sampleRate/audio.targetFPS)
		//	value = 0;
		//std::cout << "TIME : " << time << " " << value << " " << std::endl;
		//t += (double)audio.sampleRate / (double)audio.targetFPS;
		time += 1.0 / (double)audio.sampleRate;
		AudioComponent::time = time;

		for (int j = 0; j < audio.channels; j++)
		{
			audio.buffer[audio.writeCursor] = value;
			//audio.buffer[audio.writeCursor] = (float)audio_buffer[counter++] / 32767.0;
			audio.incrementWriteCursor();
		}
	}

	//assert(audio.syncCursors == false && "Audio callback did not reset syncCursors");
	audio.syncCursors = true;
}

double pianoKeyFrequency(int keyId)
{
	// Frequency of key A4 (A440) is 440 Hz
	double A4Frequency = 440.0;

	// Number of keys from A4 to the given key
	int keysDifference = keyId - 49;

	// Frequency multiplier for each semitone
	double semitoneRatio = pow(2.0, 1.0/12.0);

	// Calculate the frequency of the given key
	double frequency = A4Frequency * pow(semitoneRatio, keysDifference);

	//std::cout << keyId << " " << frequency << std::endl;

	return frequency;
}

static double freqToAngularVelocity(double hertz)
{
	return hertz * 2.0 * M_PI;
}

double osc(double hertz, double phase, double time, OscType type, double LFOHertz, double LFOAmplitude)
{
	double t = freqToAngularVelocity(hertz) * time + LFOAmplitude * hertz * (sin(freqToAngularVelocity(LFOHertz) * time)) + phase;

	switch(type)
	{
		case Sine: return sin(t);
		case Square: return sin(t) > 0 ? 1.0 : -1.0;
		case Triangle: return asin(sin(t)) * (2.0 / M_PI);
		case Saw_Ana: return 0; // [TODO] implement
		case Saw_Dig: return (2.0 / M_PI) * (hertz * M_PI * fmod(time, 1.0 / hertz) - (M_PI / 2.0));
		case Noise: return 2.0 * ((double)rand() / std::numeric_limits<int>::max()) - 1.0;
		default: return 0;
	}
}
