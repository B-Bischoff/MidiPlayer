#pragma once

#include <string>
#include <imgui.h>
#include <imgui_node_editor.h>
#include <vector>
#include "inc.hpp"

namespace ed = ax::NodeEditor;

enum UI_NodeType { NodeUI, MasterUI, NumberUI, OscUI, ADSRUI, KbFreqUI };

struct LinkInfo
{
	ed::LinkId Id;
	ed::PinId  InputId;
	ed::PinId  OutputId;
	ImColor Color;
};

struct Node;

enum class PinKind
{
	Output,
	Input
};

struct Pin
{
	ed::PinId id;
	Node* node;
	std::string name;
	//PinType type;
	PinKind kind;

	Pin()
	{
		id = 1;
		node = nullptr;
		name.clear();
		kind = PinKind::Input;
	}

	Pin(int id, const char* name):
		id(id), node(nullptr), name(name), kind(PinKind::Input)
	{
	}
};

struct Node
{
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	ImColor color;
	UI_NodeType type;

	static bool propertyChanged;

	Node()
		: id(0), name(""), inputs(), outputs(), color(ImColor(0)), type(NodeUI)
	{ }

	virtual ~Node() {}

	virtual void render()
	{
		startRender();
		renderNameAndPins();
		endRender();
	}

	void startRender()
	{
		ed::BeginNode(id);
	}

	void endRender()
	{
		ed::EndNode();
	}

	void renderNameAndPins()
	{
		ImGui::PushID(appendId(name).c_str());
		ImGui::Text("%s", name.c_str());
		ImGui::PopID();
		for (int i = 0; i < std::max(inputs.size(), outputs.size()); i++)
		{
			bool sameLine = false;

			if (i < inputs.size())
			{
				ed::BeginPin(inputs[i].id, ed::PinKind::Input);
				ImGui::Text("%s", inputs[i].name.c_str());
				ed::EndPin();
				sameLine = true;
			}
			if (i < outputs.size())
			{
				std::string spacing;

				if (sameLine)
					ImGui::SameLine();
				else if (inputs.size() > 0)
				{
					// [TODO] A better way would be to find the max length in all inputs name
					for (int j = 0; j < inputs[0].name.length(); j++)
						spacing += " ";
					spacing += " ";
				}

				ed::BeginPin(outputs[i].id, ed::PinKind::Output);
				ImGui::Text("%s%s", spacing.c_str(), outputs[i].name.c_str());
				ed::EndPin();
			}
		}
	}

	bool operator==(const Node& node)
	{
		return id == node.id;
	}

	bool operator!=(const Node& node)
	{
		return !(*this == node);
	}

protected:
	std::string appendId(const std::string& str)
	{
		return str + std::to_string(id.Get());
	}
};

struct MasterNode : public Node
{
private:
	static int nextId;
public:
	MasterNode()
	{
		id = getNextId();
		name = "Master";
		type = MasterUI;

		Pin pin;
		pin.id = getNextId(); // [TODO] create an entity managing ids
		pin.name = "> input";
		pin.node = this; // Is this really necessary ?
		pin.kind = PinKind::Input;
		inputs.push_back(pin);
	}

	static int getNextId()
	{
		return ++nextId; // Id must start at 1 and not 0
	}
};

struct NumberNode : public Node
{
	int value;

	NumberNode()
	{
		id = MasterNode::getNextId();
		name = "Number";
		type = NumberUI;

		value = 0;

		Pin pin;
		pin.id = MasterNode::getNextId();
		pin.name = "output >";
		pin.node = this;
		pin.kind = PinKind::Output;
		outputs.push_back(pin);
	}

	void render()
	{
		Node::startRender();
		Node::renderNameAndPins();

		std::string dragIntText = "Value " + std::to_string(id.Get());
		ImGui::SetNextItemWidth(50);
		if (ImGui::DragInt(dragIntText.c_str(), &value))
			Node::propertyChanged = true;

		Node::endRender();
	}
};

struct OscNode : public Node
{
	OscType oscType;
	bool doPopup = false;
	std::string popupText = "Sine";

	OscNode()
	{
		id = MasterNode::getNextId();
		name = "Osc";
		type = OscUI;

		oscType = OscType::Sine;

		Pin pin;

		pin.id = MasterNode::getNextId();
		pin.name = "> freq";
		pin.node = this;
		pin.kind = PinKind::Input;
		inputs.push_back(pin);

		pin.id = MasterNode::getNextId();
		pin.name = "> LFO Hz";
		pin.node = this;
		pin.kind = PinKind::Input;
		inputs.push_back(pin);

		pin.id = MasterNode::getNextId();
		pin.name = "> LFO Amplitude";
		pin.node = this;
		pin.kind = PinKind::Input;
		inputs.push_back(pin);

		pin.id = MasterNode::getNextId();
		pin.name = "output >";
		pin.node = this;
		pin.kind = PinKind::Output;
		outputs.push_back(pin);
	}

	void render()
	{
		Node::startRender();
		Node::renderNameAndPins();

		std::string dragIntText = "Value " + std::to_string(id.Get());
		ImGui::SetNextItemWidth(50);

		ImGui::PushID(appendId("popup").c_str());
		if (ImGui::Button(popupText.c_str()))
			doPopup = true;
		ImGui::PopID();

		Node::endRender();

		ed::Suspend();
		if (doPopup)
		{
			ImGui::OpenPopup(appendId("OscTypePopup").c_str());
			doPopup = false;
		}

		if (ImGui::BeginPopup(appendId("OscTypePopup").c_str()))
		{
			ImGui::BeginChild("popup_scroller", ImVec2(100, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			updateButtonInPopUp(OscType::Sine, "Sine", popupText);
			updateButtonInPopUp(OscType::Square, "Square", popupText);
			updateButtonInPopUp(OscType::Triangle, "Triangle", popupText);
			updateButtonInPopUp(OscType::Saw_Ana, "Saw_Ana", popupText);
			updateButtonInPopUp(OscType::Saw_Dig, "Saw_Dig", popupText);
			updateButtonInPopUp(OscType::Noise, "Noise", popupText);

			ImGui::EndChild();
			ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
		}
		ed::Resume();
	}

private:
	void updateButtonInPopUp(OscType oscType, std::string oscTypeText, std::string& popupText)
	{
		if (ImGui::Button(oscTypeText.c_str())) {
			this->oscType = oscType;
			std::cout << "--_> " << this << std::endl;
			popupText = oscTypeText;
			ImGui::CloseCurrentPopup();
			std::cout << "new type : " << oscTypeText << std::endl;

			Node::propertyChanged = true;
		}
	}
};

struct ADSR_Node : public Node {
	ADSR_Node()
	{
		id = MasterNode::getNextId();
		name = "ADSR";
		type = ADSRUI;

		Pin pin;
		pin.id = MasterNode::getNextId();
		pin.name = "output >";
		pin.node = this;
		pin.kind = PinKind::Output;
		outputs.push_back(pin);

		pin.id = MasterNode::getNextId();
		pin.name = "> input";
		pin.node = this;
		pin.kind = PinKind::Input;
		inputs.push_back(pin);
	}
};

struct KeyboardFrequencyNode : public Node
{
	KeyboardFrequencyNode()
	{
		id = MasterNode::getNextId();
		name = "Keyboard Frequency";
		type = KbFreqUI;

		Pin pin;
		pin.id = MasterNode::getNextId();
		pin.name = "frequency >";
		pin.node = this; // Is this really necessary ?
		pin.kind = PinKind::Output;

		outputs.push_back(pin);
	}
};
