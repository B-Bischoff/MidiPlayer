#include "UI/Log.hpp"

LogStream* Log::_staticStream = nullptr;

Log::Log()
	: _stream(*this)
{
	_autoScroll = true;
	_staticStream = &_stream;
	clear();
}

void Log::clear()
{
	_buf.clear();
	_entries.clear();
}

void Log::addLog(const char* fmt, ...)
{
	int old_size = _buf.size();
	va_list args;
	va_start(args, fmt);
	_buf.appendfv(fmt, args);
	va_end(args);
	std::string newEntry(_buf.begin() + old_size, _buf.end());
	parseAnsiColoredText(newEntry);
}

void Log::draw(const char* title, bool* p_open)
{
	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &_autoScroll);
		ImGui::EndPopup();
	}

	// Main window
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	bool clearLog = ImGui::Button("Clear");
	ImGui::SameLine();
	bool copy = ImGui::Button("Copy");
	ImGui::SameLine();
	_filter.Draw("Filter", -100.0f);

	ImGui::Separator();

	if (ImGui::BeginChild("scrolling", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar))
	{
		if (clearLog) clear();
		if (copy) ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		bool header = true;
		unsigned int entryIndex = 0;
		while (entryIndex < _entries.size())
		{
			const LogEntry& header = _entries[entryIndex];
			const LogEntry& log = _entries[entryIndex + 1];

			if (_filter.PassFilter((header.text + log.text).c_str()))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, header.color);
				ImGui::TextUnformatted(header.text.c_str());
				ImGui::PopStyleColor();
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Text, log.color);
				ImGui::TextUnformatted(log.text.c_str());
				ImGui::PopStyleColor();
			}

			entryIndex+=2;
		}
		ImGui::PopStyleVar();

		// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
		// Using a scrollbar or mouse-wheel will take away from the bottom edge.
		if (_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
	ImGui::End();
}

LogStream& Log::getStream()
{
	assert(_staticStream);
	return *_staticStream;
}

void Log::parseAnsiColoredText(const std::string& input) {
	// Remove ANSI color from text and put the color id in the second regex match
	std::regex ansiRegex(R"([^ -~]+\[1;([0-9]+)m)");
	std::smatch match;
	std::string remainingText = input;
	std::string currentText;
	ImVec4 currentColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white

	while (std::regex_search(remainingText, match, ansiRegex)) {
		std::string textBeforeMatch = match.prefix().str();
		std::string s(textBeforeMatch.begin() + match.position(2), textBeforeMatch.begin() + match.position(2) + match.length(2));
		if (!textBeforeMatch.empty()) {
			_entries.emplace_back(textBeforeMatch, currentColor);
		}

		std::string ansiCode = match[1].str();
		currentColor = getColorFromAnsiCode(ansiCode);

		remainingText = match.suffix().str();
	}

	// Add any remaining text after the last match
	if (!remainingText.empty())
		_entries.emplace_back(remainingText, currentColor);
}

ImVec4 Log::getColorFromAnsiCode(const std::string& code) {
	static const std::unordered_map<std::string, ImVec4> ansiToColor = {
		{"30", ImVec4(0.00, 0.00, 0.00, 1.00)}, // Black
		{"31", ImVec4(0.94, 0.17, 0.17, 1.00)}, // Red
		{"32", ImVec4(0.00, 1.00, 0.00, 1.00)}, // Green
		{"33", ImVec4(0.90, 0.84, 0.22, 1.00)}, // Yellow
		{"34", ImVec4(0.20, 0.30, 0.90, 1.00)}, // Blue
		{"35", ImVec4(0.87, 0.26, 0.90, 0.78)}, // Magenta
		{"36", ImVec4(0.00, 1.00, 1.00, 1.00)}, // Cyan
		{"37", ImVec4(1.00, 1.00, 1.00, 1.00)}, // White
	};

	auto it = ansiToColor.find(code);
	if (it != ansiToColor.end())
		return it->second;

	return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white if code is not found
}

// LogStreamBuf

LogStreamBuf::LogStreamBuf(Log& log)
	: _log(log)
{ }

int LogStreamBuf::overflow(int c)
{
	if (c != EOF)
	{
		char z = c;
		_currentLine += z;

		if (z == '\n')
		{
			_log.addLog("%s", _currentLine.c_str());
			_currentLine.clear();
		}
	}
	return c;
}

std::streamsize LogStreamBuf::xsputn(const char* s, std::streamsize n)
{
	for (std::streamsize i = 0; i < n; ++i)
		overflow(s[i]);
	return n;
}

// LogStream

LogStream::LogStream(Log& log)
	: std::ostream(&_buf), _buf(log)
{ }
