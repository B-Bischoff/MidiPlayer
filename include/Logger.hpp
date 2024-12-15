#pragma once

#include <iostream>
#include <ostream>
#include <vector>

enum LogLevel { Debug, Info, Warning, Error };

class LoggerStream {
private:
	std::vector<std::ostream*> _streams;

public:
	LoggerStream(std::vector<std::ostream*> streams) : _streams(streams) {}

	template<typename T>
	LoggerStream& operator<<(const T& t) {
		for (std::ostream* stream : _streams)
			*stream << t;
		return *this;
	}

	// Overload for std::endl and other manipulators
	LoggerStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
		for (std::ostream* stream : _streams)
			*stream << manip;
		return *this;
	}
};

class Logger {

public:
	enum colors { reset, black, red, green, yellow, blue, magenta, cyan, white };

	static LoggerStream log(const std::string& category, LogLevel level = Info, std::ostream& stream = std::cout);

	static void subscribeStream(std::ostream& stream);

private:
	static std::string _colors[9];
	static std::string* _debugColors[4];
	static std::vector<std::ostream*> _subscribedStream;

	static void setColor(LogLevel level, std::ostream& stream);
	static void resetColor(std::ostream& stream);

};
