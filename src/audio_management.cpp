#include "inc.hpp"

// [TODO] Make an entity used as an intermediate between sound generation/mixing and sound management (stream open, volume, runtime reconfiguration, ...)
int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	double *buffer = (double *) outputBuffer;
	AudioData& audio = *(AudioData*)userData;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	//std::cout << "buffer frames " << nBufferFrames << std::endl;

	for (int i = 0 ; i < nBufferFrames; i++)
	{
		double tmp;
		*buffer++ = audio.buffer[audio.leftPhase];
		if (audio.channels == 2)
			*buffer++ = audio.buffer[audio.rightPhase];
		audio.incrementPhases();
	}

	//std::cout << audio.leftPhase << " " << audio.writeCursor << std::endl;

	return 0;
}
