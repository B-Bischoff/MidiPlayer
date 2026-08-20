#include "MidiPlayer.hpp"
#include "MusicXMLParser.hpp"

int main(int argc, char* argv[])
{
	MusicXMLParser parser;
	parser.parseFile("/home/brice/Documents/MidiPlayer/convert-music-sheet/output/page-1.musicxml");
	return 0;
	MidiPlayer midiPlayer(argv[0], 1920, 1080);
	midiPlayer.update();
}
