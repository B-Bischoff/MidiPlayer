#include "AudioBackend/Instrument.hpp"
#include "AudioBackend/Components/Polyphony.hpp"
#include <unordered_set>

double Instrument::process(const AudioInfos& audioInfos)
{
	return master.getOutput(audioInfos) * volume;
}

void Instrument::findKeyboardFrequencies(const std::shared_ptr<AudioComponent>& node,
	std::vector<KeyboardFrequency*>& result,
	std::unordered_set<unsigned int>& visited)
{
	if (!node || visited.count(node->id)) return;
	visited.insert(node->id);

	if (auto* kf = dynamic_cast<KeyboardFrequency*>(node.get()))
		result.push_back(kf);

	// Also check inside Polyphony's MIDI source (it's a raw pointer, not in inputs)
	if (auto* poly = dynamic_cast<Polyphony*>(node.get()))
	{
		if (auto* kf = dynamic_cast<KeyboardFrequency*>(poly->midiSource))
			result.push_back(kf);
	}

	for (auto& inputSlot : node->inputs)
		for (auto& child : inputSlot)
			findKeyboardFrequencies(child.component, result, visited);
}

void Instrument::updateMidiSources(std::vector<MidiInfo>& keyPressed)
{
	std::vector<KeyboardFrequency*> sources;
	std::unordered_set<unsigned int> visited;

	for (auto& inputSlot : master.inputs)
		for (auto& child : inputSlot)
			findKeyboardFrequencies(child.component, sources, visited);

	for (KeyboardFrequency* src : sources)
		src->setKeyPressedRef(&keyPressed);
}
