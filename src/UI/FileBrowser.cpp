#include "UI/FileBrowser.hpp"

FileBrowser::FileBrowser()
	: _isOpen(false)
{ }

void FileBrowser::update(std::queue<Message>& messages)
{
	_browser.Display();

	if (_browser.HasSelected())
	{
		_isOpen = false;
		messages.push(Message(LOAD_AUDIO_FILE, new std::string(_browser.GetSelected().string())));
		_browser.ClearSelected();
	}
	else if (_isOpen == true && _browser.IsOpened() == false)
	{
		// Browser was close without selection
		_isOpen = false;
		messages.push(Message(CANCEL_LOAD_AUDIO_FILE));
	}
}

void FileBrowser::openFileBrowser()
{
	_isOpen = true;
	_browser.SetTitle("Load audio file");
	_browser.SetTypeFilters({ ".mp3" });
	_browser.Open();
}
