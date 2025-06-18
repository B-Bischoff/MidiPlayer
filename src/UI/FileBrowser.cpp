#include "UI/FileBrowser.hpp"

void FileBrowser::update(std::queue<Message>& messages)
{
	_browser.Display();

	if (_browser.HasSelected())
	{
		_isOpen = false;
		//messages.push(Message(LOAD_AUDIO_FILE, new std::string(_browser.GetSelected().string())));
		messages.push(Message(SEND_NODE_FILEPATH, new NodeFilepathData({_browser.GetSelected(), _callerNodeId})));
		Logger::log("filebrowser") << "load " << _browser.GetSelected() << std::endl;
		_browser.ClearSelected();
	}
	else if (_isOpen == true && _browser.IsOpened() == false)
	{
		// Browser was close without selection
		_isOpen = false;
		_callerNodeId = 0;
		Logger::log("filebrowser") << "Cancel load" << std::endl;
		//messages.push(Message(CANCEL_LOAD_AUDIO_FILE));
	}
}

void FileBrowser::openFileBrowser(const FileBrowserOpenData& data)
{
	_isOpen = true;
	_callerNodeId = data.nodeId;
	_browser.SetTitle(data.title);
	_browser.SetTypeFilters(data.filter);
	_browser.Open();
}
