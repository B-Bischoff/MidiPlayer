#pragma once

#include <string>
#include <imgui.h>
#include <imgui_node_editor.h>
#include <vector>

namespace ed = ax::NodeEditor;

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

	Node()
		: id(0), name(""), inputs(), outputs(), color(ImColor(0))
	{ }

	void render()
	{
		ed::BeginNode(id);

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

		ed::EndNode();
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

struct NodeTypeA : public Node
{
	NodeTypeA()
	{
		id = getNextId();
		name = "Node type A " + std::to_string(id.Get());

		// Input pin(s)
		for (int i = 0; i < 3; i++)
		{
			Pin pin;
			pin.id = getNextId(); // [TODO] create an entity managing ids
			pin.name = "input " + std::to_string(i);
			pin.node = this; // Is this really necessary ?
			pin.kind = PinKind::Input;

			inputs.push_back(pin);
		}

		// Output pin(s)
		for (int i = 0; i < 3; i++)
		{
			Pin pin;
			pin.id = getNextId();
			pin.name = "output " + std::to_string(i);
			pin.node = this; // Is this really necessary ?
			pin.kind = PinKind::Output;

			outputs.push_back(pin);
		}
	}
};

struct NodeTypeB : public Node
{
	NodeTypeB()
	{
		id = getNextId();
		name = "Node type A " + std::to_string(id.Get());

		// Input pin(s)
		for (int i = 0; i < 1; i++)
		{
			Pin pin;
			pin.id = getNextId(); // [TODO] create an entity managing ids
			pin.name = "input " + std::to_string(i);
			pin.node = this; // Is this really necessary ?
			pin.kind = PinKind::Input;

			inputs.push_back(pin);
		}

		// Output pin(s)
		for (int i = 0; i < 4; i++)
		{
			Pin pin;
			pin.id = getNextId();
			pin.name = "output " + std::to_string(i);
			pin.node = this; // Is this really necessary ?
			pin.kind = PinKind::Output;

			outputs.push_back(pin);
		}
	}
};
