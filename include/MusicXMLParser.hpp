#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include "pugixml.hpp"
#include "path.hpp"
#include "Logger.hpp"

struct ExpectedNote {
    int midiPitch;        // MIDI note number (60 = C4)
    double timeInBeats;   // Start time in quarter-note beats
    double timeInSeconds; // Start time in seconds
    double durationSec;   // Duration in seconds
    int velocity;         // Default or extracted velocity
};

class MusicXMLParser {
public:
	bool parseFile(fs::path filePath);

private:
	void parseMusicXML(pugi::xml_document& doc);
	int stepToMidi(char step, int alter, int octave);
	void printExpectedNotes(const std::vector<ExpectedNote>& notes);

	std::vector<ExpectedNote> _expectedNotes{};
};
