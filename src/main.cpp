#include "inc.hpp"
#include "envelope.hpp"

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration);

int main(void)
{
	std::vector<sEnvelopeADSR> envelopes(16);

	AudioData audio = {
		.sampleRate = 44100,
		.channels = 1,
		.bufferDuration = 2,
		.latency = 2,
		.buffer = nullptr,
		.leftPhase = 0, .rightPhase = 1,
		.writeCursor = 0,
		.targetFPS = 64,
	};

	const std::chrono::duration<double> targetFrameDuration(1.0f / (double)audio.targetFPS);

	audio.buffer = new float[(int)(audio.sampleRate * audio.bufferDuration) * audio.channels];

	if (audio.buffer == nullptr)
		exitError("[ERROR]: ring buffer allocation failed.");

	memset(audio.buffer, 0, sizeof(float) * audio.getBufferSize());

	rtAudioInit(audio);

	InputManager inputManager;
	initInput(inputManager);

	audio.startTime = std::chrono::high_resolution_clock::now();

	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let rtaudio get more stable [TODO] check if that is necessary
	audio.writeCursor = audio.leftPhase + ((double)audio.sampleRate / (double)audio.targetFPS) * audio.latency;
	//std::cout << "L/R/W : " << audio.leftPhase << " " << audio.rightPhase << " " << audio.writeCursor << std::endl;

	double t = 0.0;

	while (1)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		auto programElapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(startTime - audio.startTime);

		handleInput(inputManager, envelopes, t);
		generateAudio(audio, inputManager, envelopes, t);
		handleFrameProcessTime(startTime, targetFrameDuration);
	}

	// [TODO] should this be in destructor ?
	Pm_Close(inputManager.midiStream);
	Pm_Terminate();
	// [TODO] clean up audio
}

static void handleFrameProcessTime(const time_point& startTime, const std::chrono::duration<double>& targetFrameDuration)
{
	auto endTime = std::chrono::high_resolution_clock::now();
	auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

	auto sleepDuration = targetFrameDuration - deltaTime;

	if (sleepDuration > std::chrono::duration<double>(0.0))
	{
		std::this_thread::sleep_for(sleepDuration * 0.9f);
		endTime = std::chrono::high_resolution_clock::now();
		while (endTime - startTime < targetFrameDuration)
			endTime = std::chrono::high_resolution_clock::now();
	}
	else
		std::cerr << "[WARNING] : update took longer than expected" << std::endl;
}
