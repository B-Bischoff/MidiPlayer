#include "UI/UI.hpp"

void UI::updateSettings(Audio& audio, InputManager& inputManager, MidiPlayerSettings& settings)
{
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(ImGui::GetMainViewport()->Size.x * 0.5, ImGui::GetMainViewport()->Size.y * 0.8), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Settings", &_windowsState.showSettings, ImGuiWindowFlags_NoCollapse))
	{
		// Audio settings
		ImGui::SeparatorText("Audio");
		ImGui::Indent();
		updateAudioOutput(audio);
		updateAudioSampleRate(audio);
		updateAudioChannels(audio);
		updateAudioLatency(audio);
		updateMuteAudio(audio);
		ImGui::Text("\n");
		ImGui::Unindent();

		ImGui::SeparatorText("MIDI");
		ImGui::Indent();
		updateMidiSettings(inputManager, settings);
		ImGui::Text("\n");
		ImGui::Unindent();

		ImGui::SeparatorText("UI");
		ImGui::Indent();
		updateUISettings(settings);
		ImGui::Unindent();
	}

	ImGui::End();
}

void UI::updateAudioOutput(Audio& audio)
{
	// Audio output
	const RtAudio::DeviceInfo& usedDeviceInfo = audio.getUsedDeviceInfo();
	std::vector<unsigned int> deviceIds = audio.getDeviceIds();
	int currentItem = -1;

	if (deviceIds.size() == 0)
		ImGui::Text("No audio device available");
	else
	{
		std::vector<const char*> deviceNames;
		std::vector<RtAudio::DeviceInfo> deviceInfos;

		for (unsigned int deviceId : deviceIds)
		{
			RtAudio::DeviceInfo info = audio.getDeviceInfo(deviceId);

			if (info.ID == 0) continue; // Device is not available
			if (info.outputChannels == 0) continue; // Skip input only devices

			deviceInfos.push_back(info);
			deviceNames.push_back(deviceInfos.back().name.c_str());

			if (usedDeviceInfo.ID == info.ID)
				currentItem = deviceInfos.size() - 1;
		}

		if (currentItem == -1) // No audio device in use
			ImGui::Text("No audio device currently in use: ");
		else
			ImGui::Text("Currently used audio device: ");
		ImGui::SameLine();
		ImGui::PushID("ComboUsedAudioDevice");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.x * 4.0f);
		if (ImGui::Combo("", &currentItem, deviceNames.data(), deviceNames.size()))
		{
			if (audio.setAudioDevice(deviceInfos[currentItem].ID))
				ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Failed to open device %s", deviceNames[currentItem]});
			else
				ImGui::InsertNotification({ImGuiToastType::Success, 5000, "Successfully opened device %s", deviceNames[currentItem]});
		}
		ImGui::PopID();
	}
}

void UI::updateAudioSampleRate(Audio& audio)
{
	const unsigned int freq[] = { 44100, 48000 };
	const char* items[] = { "44100Hz", "48000Hz" };
	int selectedItem = audio.getSampleRate() == freq[0] ? 0 : 1;

	ImGui::Text("Sample rate");
	ImGui::PushID("SampleRateSettingCombo");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(125);
	if (ImGui::Combo("", &selectedItem, items, sizeof(items) / sizeof(const char*)))
	{
		Logger::log("Settings") << "Changed sample rate to " << items[selectedItem] << std::endl;
		if (audio.setSampleRate(freq[selectedItem]))
			ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Error changing sample rate to: %d", items[selectedItem]});
	}
	ImGui::PopID();
}

void UI::updateAudioChannels(Audio& audio)
{
	ImGui::Text("Number of channels");
	ImGui::SameLine();
	int selectedChannelMode = static_cast<int>(audio.getChannels());
	if (ImGui::RadioButton("1 - Mono", &selectedChannelMode, 1))
	{
		if (audio.setChannelNumber(1))
			ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Error changing channel(s) number to: 1"});
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("2 - Stereo", &selectedChannelMode, 2))
	{
		if (audio.setChannelNumber(2))
			ImGui::InsertNotification({ImGuiToastType::Error, 5000, "Error changing channel(s) number to: 2"});
	}
}

void UI::updateAudioLatency(Audio& audio)
{
	ImGui::Text("Audio latency (ms)");
	ImGui::SameLine();
	helpMarker("Time between audio generation and playback (in milliseconds)");

	const float space = 1.0 / 60.0 * 1000.0;
	float latency = space * audio.getLatency();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(300);
	ImGui::PushID("LatencySettingSlider");
	if (ImGui::SliderFloat("", &latency, space, space * 30, "%.3f", ImGuiSliderFlags_AlwaysClamp))
	{
		const unsigned int bufferFrameOffset = (latency / 1000.0) * 60.0;
		audio.setLatency(bufferFrameOffset);
	}

	ImGui::PopID();
	ImGui::SameLine();
	helpMarker("Click and drag to edit value.\nHold SHIFT/ALT for faster/slower edit.");
}

void UI::updateMuteAudio(Audio& audio)
{
	ImGui::Text("Mute audio");
	ImGui::SameLine();
	ImGui::PushID("MuteAudioCheckbox");
	ImGui::Checkbox("", &audio.mute);
	ImGui::PopID();
	ImGui::SameLine();
	helpMarker("Does not output sound to system but keeps updating audio generation.");
}

void UI::updateMidiSettings(InputManager& inputManager, MidiPlayerSettings& settings)
{
	const std::string currentMidiDeviceUsed = inputManager.getMidiDeviceUsed();
	const std::vector<MidiDevice> devices = inputManager.getDetectedMidiDevices();
	std::vector<const char*> deviceNames;
	int currentItem = -1;

	for (const MidiDevice& device : devices)
	{
		deviceNames.push_back(device.name.c_str());
		if (device.name == currentMidiDeviceUsed)
			currentItem = deviceNames.size() - 1;
	}

	if (deviceNames.size() == 0)
	{
		ImGui::Text("No MIDI device found");
	}
	else
	{
		if (currentItem == -1)
			ImGui::Text("No MIDI device in use");
		else
			ImGui::Text("Currently used MIDI device:");
		ImGui::SameLine();

		// Estimate button width
		const float buttonWidth = ImGui::CalcTextSize("Close MIDI device").x + ImGui::GetStyle().FramePadding.x * 8.0f;
		float comboWidth = ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x;

		ImGui::PushID("UsedMidiDeviceCombo");
		ImGui::SetNextItemWidth(comboWidth > 0.0f ? comboWidth : 1.0f); // avoid negative
		if (ImGui::Combo("", &currentItem, deviceNames.data(), deviceNames.size()))
		{
			inputManager.setMidiDeviceUsed(deviceNames[currentItem]);

			// Logic to automatically disable keyboard as MIDI input
			if (currentMidiDeviceUsed.empty() && settings.useKeyboardAsInput)
				settings.useKeyboardAsInput = false;
		}
		ImGui::PopID();
		ImGui::SameLine();
		if (currentMidiDeviceUsed.empty())
			ImGui::BeginDisabled();
		if (ImGui::Button("Close MIDI device"))
		{
			inputManager.closeMidiDevice();

			// Logic to automatically enable keyboard as MIDI input
			if (!currentMidiDeviceUsed.empty() && !settings.useKeyboardAsInput)
				settings.useKeyboardAsInput = true;
		}
		if (currentMidiDeviceUsed.empty())
			ImGui::EndDisabled();
	}

	ImGui::Text("Use keyboard as MIDI input");
	ImGui::SameLine();
	helpMarker("Using keyboard keys are:\n S D     G H J\nZ X C V B N M\nTo change octave: O / P");
	ImGui::SameLine();
	ImGui::PushID("UseKeyboardSettings");
	ImGui::Checkbox("", &settings.useKeyboardAsInput);
	ImGui::PopID();
}

void UI::updateUISettings(MidiPlayerSettings& settings)
{
	ImGuiIO& io = ImGui::GetIO();
	float fontScale = io.FontGlobalScale;
	ImGui::Text("Font scale ");
	ImGui::SameLine();
	ImGui::PushID("FontScaleDragFloat");
	if (ImGui::DragFloat("", &fontScale, 0.005, 0.3, 1.5))
		io.FontGlobalScale = fontScale;
	ImGui::PopID();

	ImGui::Text("Separate left/right buffer graph");
	ImGui::SameLine();
	ImGui::PushID("SplitGraphCheckBox");
	ImGui::Checkbox("", &settings.splitBufferGraph);
	ImGui::PopID();
}
