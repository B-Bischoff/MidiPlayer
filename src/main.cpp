#include "MidiPlayer.hpp"

int main(int argc, char* argv[])
{
	MidiPlayer midiPlayer(argv[0], 1920, 1080);
	midiPlayer.update();
}
