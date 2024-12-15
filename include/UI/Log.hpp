#pragma once

#include <streambuf>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <imgui.h>

class Log;

class LogStreamBuf : public std::streambuf {
public:
	LogStreamBuf(Log& log);

protected:
	// Override the overflow function to handle characters.
	int overflow(int c) override;
	// Override xsputn to handle writing multiple characters at once.
	std::streamsize xsputn(const char* s, std::streamsize n) override;

private:
	Log& _log;
	std::string _currentLine; // Buffer to hold the current line until a newline is encountered.
};

struct LogEntry {
	std::string text;
	ImVec4 color;

	LogEntry(const std::string& text, const ImVec4& color)
		: text(text), color(color) {}
};


class LogStream : public std::ostream
{
public:
	LogStream(Log& log);

private:
	LogStreamBuf _buf;
};

class Log {
public:
	ImGuiTextBuffer _buf;
	ImGuiTextFilter _filter;
	bool _autoScroll; // Keep scrolling if already at the bottom.
	std::vector<LogEntry> _entries;

	LogStream _stream;
	static LogStream* _staticStream;

public:
	Log();

	void clear();
	void addLog(const char* fmt, ...) IM_FMTARGS(2);
	void draw(const char* title, bool* p_open = NULL);
	static LogStream& getStream();

private:
	ImVec4 getColorFromAnsiCode(const std::string& code);
	void parseAnsiColoredText(const std::string& input);
};
