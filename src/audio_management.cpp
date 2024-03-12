#include "inc.hpp"

// [TODO] Make an entity used as an intermediate between sound generation/mixing and sound management (stream open, volume, runtime reconfiguration, ...)
int uploadBuffer(void *outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
{
	static unsigned int READ_COUNT = 0;
	double *buffer = (double *) outputBuffer;
	AudioData& audio = *(AudioData*)userData;

	//std::cout << "callback time : " << streamTime << std::endl;
	if (status)
		std::cout << "Stream underflow detected!" << std::endl;

	READ_COUNT += nBufferFrames;

	//std::cout << "buffer frames " << nBufferFrames << std::endl;

	for (int i = 0 ; i < nBufferFrames; i++)
	{
		double tmp;
		*buffer++ = audio.buffer[audio.leftPhase];
		if (audio.channels == 2)
			*buffer++ = audio.buffer[audio.rightPhase];
		audio.incrementPhases();
	}

	if (audio.set)
	{
		audio.set = false;
		//std::cout << "OFF" << std::endl;
		int delta = audio.writeCursor - audio.leftPhase;
		if (delta < 0)
			delta += audio.getBufferSize();
		//std::cout << "delta : " << delta << std::endl;
		audio.samplesToRecover = 735*2 - delta;
	}

	//std::cout << "READ_COUNT : " << READ_COUNT << std::endl;
	//std::cout << "READ THEORIC TIME : " << (double)READ_COUNT/44100.0 << std::endl;
	//std::cout << audio.leftPhase << " " << audio.writeCursor << std::endl;

	return 0;
}
