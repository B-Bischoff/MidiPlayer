#include "AudioBackend/Components/AudioComponent.hpp"
#include "AudioBackend/Components/MidiSourceComponent.hpp"

bool AudioComponent::isMidiSource() const
{
	return dynamic_cast<const MidiSourceComponent*>(this) != nullptr;
}

std::shared_ptr<AudioComponent> AudioComponent::deepClone() const
{
	auto copy = clone();
	copy->inputs.resize(inputs.size());

	for (size_t i = 0; i < inputs.size(); i++)
	{
		for (const auto& child : inputs[i])
		{
			// Skip MIDI sources — they are shared, not cloned
			if (child->isMidiSource())
				continue;
			copy->inputs[i].push_back(child->deepClone());
		}
	}

	return copy;
}
