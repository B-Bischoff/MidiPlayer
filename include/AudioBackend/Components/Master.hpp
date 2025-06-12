#pragma once

#include <unordered_set>
#include "AudioComponent.hpp"
#include "audio_backend.hpp"

struct Master : public AudioComponent {
private:
	bool showWarning = true;

public:
	enum Inputs { input };

	Master() : AudioComponent() { inputs.resize(1); componentName = "Master"; }

	virtual ~Master()
	{
		Components components = getInputs();
		std::unordered_set<int> ids;

		// Store all direct childs ids
		for (AudioComponent* component : components)
			ids.insert(component->id);

		// It is important to check if child still exists before deleting it
		// Following pattern shows why:
		//
		//  child2 ──────────────┐
		//    │                  v
		//    └──> child1 ───> master
		//
		// If Child 1 is deleted first, it would also delete child2
		for (const int id : ids)
		{
			AudioComponent* component = getAudioComponent(id);
			if (component)
				deleteComponentAndInputs(component);
		}
	}

	double process(PipelineInfo& info) override
	{
		if (!inputs.size())
		{
			if (showWarning)
			{
				showWarning = false;
				Logger::log("Audio", Warning) << "No input plugged to master." << std::endl;
			}
			return 0;
		}

		showWarning = true;

		double value = 0.0;


		info.pipelineInitiator = id;
		for (AudioComponent* input : inputs[input])
		{
			info.currentKey = 0;
			do
			{
				value += input->process(info);
			} while (++info.currentKey < info.keyPressed.size());
		}

		return value;
	}

	void deleteComponentAndInputs(AudioComponent* component)
	{
		Components inputs = component->getInputs();

		auto it = inputs.begin();
		while (it != inputs.end())
		{
			if (idExists((*it)->id))
				deleteComponentAndInputs(*it);
			else
				it++;
			inputs = component->getInputs();
			it = inputs.begin();
			if (it == inputs.end())
				break;
		}

		removeComponentFromBranch(component, true);
	}
};
