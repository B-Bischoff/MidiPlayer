#include "MusicXMLParser.hpp"

bool MusicXMLParser::parseFile(fs::path filePath)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(filePath.c_str());

	if (!result)
	{
		Logger::log("MusicXMLParser", Error) << "Failed to parse XML file: " << result.description() << std::endl;
		return false;
	}

	parseMusicXML(doc);
	if (_expectedNotes.empty())
	{
		Logger::log("MusicXMLParser", Warning) << "No notes found in the MusicXML file." << std::endl;
		return false;
	}

	printExpectedNotes(_expectedNotes);

	return true;
}

void MusicXMLParser::parseMusicXML(pugi::xml_document& doc)
{
	// Locate the first part in score-partwise
	pugi::xml_node part = doc.child("score-partwise").child("part");
	if (!part)
	{
		Logger::log("MusicXMLParser", Error) << "No <part> element found in MusicXML!" << std::endl;
		return ;
	}

	int divisions = 1;				// Divisions per quarter note (MusicXML default setup)
	double bpm = 120.0;				// Default fallback tempo
	double currentBeat = 0.0;		// Global time tracker in quarter notes
	double currentSeconds = 0.0;	// Global time tracker in seconds

	// Iterate through measures
	for (pugi::xml_node measure : part.children("measure"))
	{
		// Track previous note duration to handle chord timing correctly
		int lastNoteDuration = 0;

		for (pugi::xml_node child : measure.children())
		{
			std::string nodeName = child.name();

			// 1. Update Divisions (how duration units map to quarter notes)
			if (nodeName == "attributes")
			{
				pugi::xml_node divNode = child.child("divisions");
				if (divNode)
					divisions = divNode.text().as_int(1);
			}

			// 2. Update Tempo (BPM)
			else if (nodeName == "direction")
			{
				pugi::xml_node soundNode = child.child("sound");
				if (soundNode && soundNode.attribute("tempo"))
					bpm = soundNode.attribute("tempo").as_double(120.0);
			}

			// 3. Process Notes and Rests
			else if (nodeName == "note")
			{
				int durationTicks = child.child("duration").text().as_int(0);
				bool isChord = (child.child("chord") != nullptr);
				bool isRest = (child.child("rest") != nullptr);

				// If it's a chord, rewind time to the start of the previous note
				if (isChord)
				{
					double chordOffsetTicks = durationTicks; // Rewind duration
					double chordOffsetBeats = static_cast<double>(lastNoteDuration) / divisions;
					double chordOffsetSec = chordOffsetBeats * (60.0 / bpm);

					currentBeat -= chordOffsetBeats;
					currentSeconds -= chordOffsetSec;
				}

				double noteDurationBeats = static_cast<double>(durationTicks) / divisions;
				double noteDurationSec = noteDurationBeats * (60.0 / bpm);

				// If it's a playable pitch (not a rest)
				if (!isRest)
				{
					pugi::xml_node pitchNode = child.child("pitch");
					if (pitchNode)
					{
						char step = pitchNode.child("step").text().get()[0];
						int alter = pitchNode.child("alter").text().as_int(0);
						int octave = pitchNode.child("octave").text().as_int(4);

						int midiPitch = stepToMidi(step, alter, octave);

						ExpectedNote note;
						note.midiPitch = midiPitch;
						note.timeInBeats = currentBeat;
						note.timeInSeconds = currentSeconds;
						note.durationSec = noteDurationSec;
						note.velocity = 80;

						_expectedNotes.push_back(note);
					}
				}

				// Advance global timeline
				currentBeat += noteDurationBeats;
				currentSeconds += noteDurationSec;
				lastNoteDuration = durationTicks;
			}
		}
	}
}

int MusicXMLParser::stepToMidi(char step, int alter, int octave)
{
	int base = 0;
	switch (step) {
		case 'C': base = 0;  break;
		case 'D': base = 2;  break;
		case 'E': base = 4;  break;
		case 'F': base = 5;  break;
		case 'G': base = 7;  break;
		case 'A': base = 9;  break;
		case 'B': base = 11; break;
		default:  return -1;
	}
	return (octave + 1) * 12 + base + alter;
}

void MusicXMLParser::printExpectedNotes(const std::vector<ExpectedNote>& notes)
{
	std::cout << "Successfully extracted " << notes.size() << " notes:\n\n";
	std::cout << "MIDI Pitch\tBeat Offset\tTimestamp (s)\tDuration (s)\n";
	std::cout << "---------------------------------------------------------\n";

	for (const auto& note : notes) {
		std::cout << note.midiPitch << "\t\t"
				  << note.timeInBeats << "\t\t"
				  << note.timeInSeconds << " s\t\t"
				  << note.durationSec << " s\n";
	}
}
