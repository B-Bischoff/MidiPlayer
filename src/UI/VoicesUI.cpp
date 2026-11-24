#include <cfloat>
#include <cstdio>
#include "UI/VoicesUI.hpp"

namespace {
	constexpr ImVec4 heldColor      = ImVec4(0.40f, 0.80f, 0.45f, 1.0f);
	constexpr ImVec4 releasingColor = ImVec4(0.95f, 0.65f, 0.25f, 1.0f);
	constexpr ImVec4 stealingColor  = ImVec4(0.90f, 0.35f, 0.35f, 1.0f);
	constexpr ImVec4 freeColor      = ImVec4(0.45f, 0.45f, 0.50f, 1.0f);

	// A steal fade (a few ms) is shorter than a UI frame: highlight the row for a while after it instead
	constexpr double stealHighlightSeconds = 0.3;
}

void VoicesUI::draw(const char* title, Instrument* instrument, bool* open)
{
	if (!ImGui::Begin(title, open, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}

	int polyphonyCount = 0;
	if (instrument)
	{
		// The Compiler always places Polyphony nodes directly on Master's inputs
		for (auto& slot : instrument->master.inputs)
		{
			for (auto& connection : slot)
			{
				if (auto* polyphony = dynamic_cast<const Polyphony*>(connection.component.get()))
					drawPolyphony(*polyphony, polyphonyCount++);
			}
		}
	}

	if (polyphonyCount == 0)
		ImGui::TextDisabled("No polyphonic voice (connect a Keyboard node to Master)");

	ImGui::End();
}

void VoicesUI::drawPolyphony(const Polyphony& polyphony, int index)
{
	int held = 0, releasing = 0;
	for (const Polyphony::Voice& voice : polyphony.voices)
	{
		held += voice.active;
		releasing += voice.releasing;
	}

	ImGui::PushID(index);
	ImGui::SeparatorText(("Polyphony " + std::to_string(index + 1)).c_str());
	ImGui::Text("Held: %d   Releasing: %d   Free: %d   Steals: %lu",
		held, releasing, (int)polyphony.voices.size() - held - releasing, polyphony.stealCount);

	const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("voices", 6, flags))
	{
		ImGui::TableSetupColumn("#");
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Note");
		ImGui::TableSetupColumn("Vel");
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 140.0f);
		ImGui::TableSetupColumn("Release stages", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (size_t i = 0; i < polyphony.voices.size(); i++)
			drawVoiceRow(polyphony.voices[i], (int)i);

		ImGui::EndTable();
	}
	ImGui::PopID();
}

void VoicesUI::drawVoiceRow(const Polyphony::Voice& voice, int index)
{
	const bool isFree = !voice.active && !voice.releasing;

	ImGui::TableNextRow();

	const double sinceSteal = AudioComponent::time - voice.lastStealTime;
	if (voice.stealing || sinceSteal < stealHighlightSeconds)
	{
		const float alpha = voice.stealing ? 0.5f : 0.5f * (float)(1.0 - sinceSteal / stealHighlightSeconds);
		ImVec4 color = stealingColor;
		color.w = alpha;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(color));
	}

	ImGui::TableNextColumn();
	ImGui::Text("%d", index);

	ImGui::TableNextColumn();
	if (voice.stealing)
		ImGui::TextColored(stealingColor, "stolen");
	else if (voice.active)
		ImGui::TextColored(heldColor, "held");
	else if (voice.releasing)
		ImGui::TextColored(releasingColor, "releasing");
	else
		ImGui::TextColored(freeColor, "free");

	ImGui::TableNextColumn();
	if (isFree)
		ImGui::TextDisabled("-");
	else if (voice.stealing)
	{
		// The current note fades out, the pending one starts afterwards
		const std::string pending = voice.pendingNote >= 0 ? noteName(voice.pendingNote) : "none";
		ImGui::Text("%s -> %s", noteName(voice.noteId).c_str(), pending.c_str());
	}
	else
		ImGui::Text("%s (%d)", noteName(voice.noteId).c_str(), voice.noteId);

	ImGui::TableNextColumn();
	if (isFree)
		ImGui::TextDisabled("-");
	else if (voice.stealing)
		ImGui::Text("%d", voice.pendingVelocity);
	else
		ImGui::Text("%d", voice.info.velocity);

	ImGui::TableNextColumn();
	if (voice.stealing)
		ImGui::TextColored(stealingColor, "fading");
	else if (voice.active)
		ImGui::Text("%.1f s", AudioComponent::time - voice.assignTime);
	else if (voice.releasing)
	{
		// Progress towards the safety release limit: a full bar means the
		// release stages never reported finished
		const double elapsed = AudioComponent::time - voice.releaseStartTime;
		char label[32];
		std::snprintf(label, sizeof(label), "%.1f s", elapsed);
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, releasingColor);
		ImGui::ProgressBar((float)(elapsed / Polyphony::MAX_RELEASE_SECONDS), ImVec2(-FLT_MIN, 0.0f), label);
		ImGui::PopStyleColor();
	}
	else
		ImGui::TextDisabled("-");

	ImGui::TableNextColumn();
	if (voice.releaseNodes.empty())
		ImGui::TextDisabled("none (stops on key release)");
	else
	{
		std::string states;
		for (const AudioComponent* node : voice.releaseNodes)
		{
			if (!states.empty())
				states += ", ";
			states += releaseNodeState(node);
		}
		if (isFree)
			ImGui::TextDisabled("%s", states.c_str());
		else
			ImGui::TextUnformatted(states.c_str());
	}
}

std::string VoicesUI::noteName(int midiNote)
{
	static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	if (midiNote < 0)
		return "-";
	return names[midiNote % 12] + std::to_string(midiNote / 12 - 1);
}

std::string VoicesUI::releaseNodeState(const AudioComponent* node)
{
	if (auto* adsr = dynamic_cast<const ADSR*>(node))
	{
		static const char* phases[] = { "Attack", "Decay", "Sustain", "Release", "Inactive", "Retrigger" };
		return "ADSR: " + std::string(phases[adsr->voiceEnvelope.phase]);
	}
	if (auto* soundFont = dynamic_cast<const SoundFontPlayer*>(node))
		return "SoundFont: " + std::to_string(soundFont->activeVoiceCount()) + " tsf voice(s)";

	return node->componentName + (node->isReleaseFinished() ? ": done" : ": running");
}
