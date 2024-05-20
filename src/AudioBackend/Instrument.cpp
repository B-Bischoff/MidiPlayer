#include "AudioBackend/Instrument.hpp"

double Instrument::process(std::vector<MidiInfo>& keyPressed)
{
	return master.process(keyPressed) * volume;
}
