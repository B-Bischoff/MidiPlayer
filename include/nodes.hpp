#pragma once

#include <string>
#include <imgui.h>
#include <imgui_node_editor.h>
#include <vector>
#include "inc.hpp"

namespace ed = ax::NodeEditor;

enum UI_NodeType { NodeUI, MasterUI, NumberUI, OscUI, ADSRUI, KbFreqUI };

int nextId = 0;

int getNextId()
{
	return ++nextId;
}

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
		ImGui::Text("%s", name.c_str());
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
};

struct MasterNode : public Node
{
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
};

struct NumberNode : public Node
{
	int value;

	NumberNode()
	{
		id = getNextId();
		name = "Number " + std::to_string(id.Get());
		type = NumberUI;

		value = 0;

		Pin pin;
		pin.id = getNextId();
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
		ImGui::DragInt(dragIntText.c_str(), &value);

		Node::endRender();
	}
};

struct OscNode : public Node
{
	OscType oscType;

	OscNode()
	{
		id = getNextId();
		name = "Osc " + std::to_string(id.Get());
		type = OscUI;

		oscType = OscType::Sine;

		Pin pin;

		pin.id = getNextId();
		pin.name = "> input";
		pin.node = this;
		pin.kind = PinKind::Input;
		inputs.push_back(pin);

		pin.id = getNextId();
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

		static std::string popupText = "Sine";
		static bool doPopup = false;
		if (ImGui::Button(popupText.c_str()))
			doPopup = true;

		Node::endRender();

		ed::Suspend();
		if (doPopup)
		{
			ImGui::OpenPopup("OscTypePopup");
			doPopup = false;
		}

		if (ImGui::BeginPopup("OscTypePopup"))
		{
			ImGui::TextDisabled("Pick One:");
			ImGui::BeginChild("popup_scroller", ImVec2(100, 100), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
			if (ImGui::Button("Sine")) {
				popupText = "Sine";
				ImGui::CloseCurrentPopup();  // These calls revoke the popup open state, which was set by OpenPopup above.
			}
			if (ImGui::Button("Square")) {
				popupText = "Square";
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Triangle")) {
				popupText = "Triangle";
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::Button("Saw_Ana")) {
				popupText = "Saw_Ana";
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndChild();
			ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
		}
		ed::Resume();
	}
};

struct ADSR_Node : public Node {
	ADSR_Node()
	{
		id = getNextId();
		name = "ADSR " + std::to_string(id.Get());
		type = ADSRUI;

		Pin pin;
		pin.id = getNextId();
		pin.name = "output >";
		pin.node = this;
		pin.kind = PinKind::Output;
		outputs.push_back(pin);

		pin.id = getNextId();
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
		id = getNextId();
		name = "Keyboard Frequency " + std::to_string(id.Get());
		type = KbFreqUI;

		Pin pin;
		pin.id = getNextId();
		pin.name = "frequency >";
		pin.node = this; // Is this really necessary ?
		pin.kind = PinKind::Output;

		outputs.push_back(pin);
	}
};
