#pragma once

#include <string>
#include <imgui.h>
#include <vector>
#include "inc.hpp"

#include "cereal/types/polymorphic.hpp"

namespace ed = ax::NodeEditor;

enum UI_NodeType { NodeUI, MasterUI, NumberUI, OscUI, ADSRUI, KbFreqUI, MultUI, LowPassUI};

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
	unsigned int id;
	Node* node;
	std::string name;
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
	{ }
};

struct Node
{
private:
	static int nextId; // [TODO] create an entity managing ids

public:
	unsigned int id;
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

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("node_id", id),
			cereal::make_nvp("node_name", name),
			cereal::make_nvp("node_type", (int)type),
			cereal::make_nvp("node_inputs", inputs),
			cereal::make_nvp("node_outputs", outputs)
			// cereal::make_nvp("color", color) // [TODO] work on node/link colors
		);
	}

	bool operator==(const Node& node)
	{
		return id == node.id;
	}

	bool operator!=(const Node& node)
	{
		return !(*this == node);
	}

	static int getNextId()
	{
		return ++nextId; // Id must start at 1 and not 0
	}

protected:
	std::string appendId(const std::string& str)
	{
		return str + std::to_string(id);
	}

	Pin createPin(const std::string& name, PinKind kind)
	{
		Pin pin;
		pin.id = getNextId();
		pin.name = name;
		pin.node = this; // Is this really necessary ?
		pin.kind = kind;

		return pin;
	}
};

struct MasterNode : public Node
{
public:
	MasterNode()
	{
		id = getNextId();
		name = "Master";
		type = MasterUI;

		inputs.push_back(createPin("> input", PinKind::Input));
	}
};

struct NumberNode : public Node
{
	float value;

	NumberNode()
	{
		id = MasterNode::getNextId();
		name = "Number";
		type = NumberUI;

		value = 0.0f;
		outputs.push_back(createPin("output >", PinKind::Output));
	}

	void render()
	{
		Node::startRender();
		Node::renderNameAndPins();

		ImGui::SetNextItemWidth(50);
		ImGui::PushID(appendId("NumberValue").c_str());
		if (ImGui::DragFloat("Value", &value, 0.001))
			Node::propertyChanged = true;
		ImGui::PopID();

		Node::endRender();
	}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("base_node", cereal::base_class<Node>(this)),
			cereal::make_nvp("number_value", value)
		);
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

		inputs.push_back(createPin("> freq", PinKind::Input));
		inputs.push_back(createPin("> phase", PinKind::Input));
		inputs.push_back(createPin("> LFO Hz", PinKind::Input));
		inputs.push_back(createPin("> LFO Amplitude", PinKind::Input));
		outputs.push_back(createPin("output >", PinKind::Output));
	}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("base_node", cereal::base_class<Node>(this)),
			cereal::make_nvp("oscillator_type", oscType)
		);
	}

	void render()
	{
		Node::startRender();
		Node::renderNameAndPins();

		std::string dragIntText = "Value " + std::to_string(id);
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

		outputs.push_back(createPin("output >", PinKind::Output));
		inputs.push_back(createPin("> input", PinKind::Input));
	}
};

struct KeyboardFrequencyNode : public Node {
	KeyboardFrequencyNode()
	{
		id = MasterNode::getNextId();
		name = "Keyboard Frequency";
		type = KbFreqUI;

		outputs.push_back(createPin("frequency >", PinKind::Output));
	}
};

struct MultNode : public Node {
	MultNode()
	{
		id = MasterNode::getNextId();
		name = "Multiplier";
		type = MultUI;

		inputs.push_back(createPin("> input A", PinKind::Input));
		inputs.push_back(createPin("> input B", PinKind::Input));
		outputs.push_back(createPin("output >", PinKind::Output));
	}
};

struct LowPassFilterNode : public Node {
	LowPassFilterNode()
	{
		id = MasterNode::getNextId();
		name = "Low Pass Filter";
		type = LowPassUI;
		inputs.push_back(createPin("> signal", PinKind::Input));
		inputs.push_back(createPin("> alpha", PinKind::Input));
		outputs.push_back(createPin("output >", PinKind::Output));
	}
};

// Register every Node child classes
CEREAL_REGISTER_TYPE(MasterNode)
CEREAL_REGISTER_TYPE(NumberNode)
CEREAL_REGISTER_TYPE(OscNode)
CEREAL_REGISTER_TYPE(ADSR_Node)
CEREAL_REGISTER_TYPE(KeyboardFrequencyNode)
CEREAL_REGISTER_TYPE(MultNode)
CEREAL_REGISTER_TYPE(LowPassFilterNode)

// Register child class if it does not have serialization method.
// This indicate cereal to use Node serialization method.
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, MasterNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, ADSR_Node)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, KeyboardFrequencyNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, MultNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, LowPassFilterNode)
