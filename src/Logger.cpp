#include "Logger.hpp"

std::string Logger::_colors[9] = {
	ANSI_RESET,
	ANSI_FG_BLACK,
	ANSI_FG_RED,
	ANSI_FG_GREEN,
	ANSI_FG_YELLOW,
	ANSI_FG_BLUE,
	ANSI_FG_MAGENTA,
	ANSI_FG_CYAN,
	ANSI_FG_WHITE,
};
std::string* Logger::_debugColors[4] = {
	&_colors[magenta], // debug
	&_colors[blue], // info
	&_colors[yellow], // warning
	&_colors[red], // error
};
std::vector<std::ostream*> Logger::_subscribedStream;

LoggerStream Logger::log(const std::string& category, LogLevel level, std::ostream& stream)
{
	std::vector streams = _subscribedStream;
	streams.push_back(&stream);

	for (std::ostream* currentStream : streams)
	{
		setColor(level, *currentStream);
		*currentStream << "[" << category << "] ";
		resetColor(*currentStream);
	}
	return LoggerStream(streams);
}

void Logger::setColor(LogLevel level, std::ostream& stream)
{
	stream << *_debugColors[level];
}

void Logger::resetColor(std::ostream& stream)
{
	stream << _colors[reset];
}

void Logger::subscribeStream(std::ostream& stream)
{
	for (std::ostream* s : _subscribedStream)
	{
		if (s == &stream)
			log("Logger subscription", Warning) << "Registering an already subscribed stream" << std::endl;
	}
	_subscribedStream.push_back(&stream);
}
