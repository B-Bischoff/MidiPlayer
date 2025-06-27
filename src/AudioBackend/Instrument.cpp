#include "AudioBackend/Instrument.hpp"

double Instrument::process(const AudioInfos& audioInfos, std::vector<MidiInfo>& keyPressed)
{
	return master.process(audioInfos, keyPressed) * volume;
}
