#include "Logger.hpp"

std::string Logger::_colors[9] = {
	"\033[1;0m", // reset
	"\033[1;30m", // black
	"\033[1;31m", // red
	"\033[1;32m", // green
	"\033[1;33m", // yellow
	"\033[1;34m", // blue
	"\033[1;35m", // magenta
	"\033[1;36m", // cyan
	"\033[1;37m", // white
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
