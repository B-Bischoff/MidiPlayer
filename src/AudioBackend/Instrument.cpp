#include "AudioBackend/Instrument.hpp"

double Instrument::process(PipelineInfo& info)
{
	return master.process(info) * volume;
}
