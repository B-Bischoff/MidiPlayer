#include "UI/FileBrowser.hpp"

void FileBrowser::update(std::queue<Message>& messages)
{
	_browser.Display();

	if (_browser.HasSelected())
	{
		_isOpen = false;
		messages.push(Message(SEND_NODE_FILEPATH, new NodeFilepathData({_browser.GetSelected(), _callerNodeId})));
		_browser.ClearSelected();
	}
	else if (_isOpen == true && _browser.IsOpened() == false)
	{
		// Browser was close without selection
		_isOpen = false;
		_callerNodeId = 0;
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
