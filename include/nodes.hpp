#pragma once

#include <string>
#include <vector>
#include <imgui.h>
#include "UI/IDManager.hpp"
#include "inc.hpp"
#include "UI/Message.hpp"
#include "MidiMath.hpp"

#include "AudioBackend/Components/Components.hpp"

#include "cereal/types/polymorphic.hpp"

namespace ed = ax::NodeEditor;

enum UI_NodeType { NodeUI, MasterUI, NumberUI, OscUI, ADSRUI, KbFreqUI, MultUI, LowPassUI, CombFilterUI};

struct LinkInfo
{
	ed::LinkId Id;
	ed::PinId InputId;
	ed::PinId OutputId;
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
	enum Mode { Link, Slider};

	unsigned int id;
	Node* node;
	std::string name;
	PinKind kind;

	Mode mode;
	float* sliderValue;

	Pin()
	{
		id = 1;
		node = nullptr;
		name.clear();
		kind = PinKind::Input;
		mode = Mode::Link;
		sliderValue = nullptr;
	}

	Pin(int id, const char* name):
		id(id), node(nullptr), name(name), kind(PinKind::Input), mode(Mode::Link), sliderValue(nullptr)
	{ }
};

struct Node
{
	unsigned int id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;
	ImColor color;
	UI_NodeType type;
	bool hidden;

	static bool propertyChanged;

	Node()
		: id(0), name(""), inputs(), outputs(), color(ImColor(0)), type(NodeUI), hidden(false)
	{ }

	virtual ~Node() {}

	virtual AudioComponent* convertNodeToAudioComponent() = 0;

	virtual void render(std::queue<Message>& messages)
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
#if NODE_DEBUG
		ImGui::Text("%s", (name + "_" + std::to_string(id)).c_str());
#else
		ImGui::Text("%s", name.c_str());
#endif
		ImGui::PopID();
		for (int i = 0; i < std::max(inputs.size(), outputs.size()); i++)
		{
			bool sameLine = false;

			if (i < inputs.size())
			{
				ed::BeginPin(inputs[i].id, ed::PinKind::Input);
				if (inputs[i].mode == Pin::Mode::Link)
				{
#if NODE_DEBUG
					ImGui::Text("%s", (inputs[i].name + "_" + std::to_string(inputs[i].id)).c_str());
#else
					ImGui::Text("%s", inputs[i].name.c_str());
#endif
				}
				else
				{
					ImGui::SetNextItemWidth(50);
					ImGui::PushID(appendId(inputs[i].name).c_str());
					if (ImGui::DragFloat(inputs[i].name.c_str(), inputs[i].sliderValue, 0.001))
						Node::propertyChanged = true;
					ImGui::PopID();
				}
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
#if NODE_DEBUG
				ImGui::Text("%s%s", spacing.c_str(), (outputs[i].name + "_" + std::to_string(outputs[i].id)).c_str());
#else
				ImGui::Text("%s%s", spacing.c_str(), outputs[i].name.c_str());
#endif
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
			cereal::make_nvp("node_outputs", outputs),
			cereal::make_nvp("node_hidden", hidden)
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

protected:
	std::string appendId(const std::string& str)
	{
		return str + std::to_string(id);
	}

	Pin createPin(IDManager* idManager, const std::string& name, PinKind kind)
	{
		Pin pin;
		pin.id = INVALID_ID;
		if (idManager != nullptr)
			pin.id = idManager->getID();
		pin.name = name;
		pin.node = this; // Is this really necessary ?
		pin.kind = kind;

		return pin;
	}

	unsigned int getId(IDManager* idManager)
	{
		if (idManager != nullptr)
			return idManager->getID();
		else
			return INVALID_ID;
	}
};

struct MasterNode : public Node
{
	MasterNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Master";
		type = MasterUI;

		inputs.push_back(createPin(idManager, "> input", PinKind::Input));
	}

	AudioComponent* convertNodeToAudioComponent() override { return new Master; }
};

struct NumberNode : public Node
{
	float value;

	NumberNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Number";
		type = NumberUI;

		value = 0.0f;
		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override
	{
		Number* audioComponent = new Number;
		audioComponent->number = value;
		return audioComponent;
	}

	void render(std::queue<Message>& messages) override
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
	static const int oscTypeNumber = 6;
	std::string popupText[oscTypeNumber] = {"Sine", "Square", "Triangle", "Saw_Ana", "Saw_Dig", "Noise"};

	OscNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Osc";
		type = OscUI;

		oscType = OscType::Sine;

		inputs.push_back(createPin(idManager, "> freq", PinKind::Input));
		inputs.push_back(createPin(idManager, "> phase", PinKind::Input));
		inputs.push_back(createPin(idManager, "> LFO Hz", PinKind::Input));
		inputs.push_back(createPin(idManager, "> LFO Amplitude", PinKind::Input));
		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override
	{
		Oscillator* audioComponent = new Oscillator;
		audioComponent->type = oscType;
		return audioComponent;
	}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("base_node", cereal::base_class<Node>(this)),
			cereal::make_nvp("oscillator_type", oscType)
		);
	}

	void render(std::queue<Message>& messages) override
	{
		Node::startRender();
		Node::renderNameAndPins();

		ImGui::SetNextItemWidth(50);

		ImGui::PushID(appendId("popup").c_str());
		if (ImGui::Button(popupText[(int)oscType].c_str()))
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

			for (int i = 0; i < oscTypeNumber; i++)
			{
				if (ImGui::Button(popupText[i].c_str()))
				{
					this->oscType = static_cast<OscType>(i);
					ImGui::CloseCurrentPopup();
					Node::propertyChanged = true;
				}
			}

			ImGui::EndChild();
			ImGui::EndPopup(); // Note this does not do anything to the popup open/close state. It just terminates the content declaration.
		}
		ed::Resume();
	}
};

struct ADSR_Node : public Node {
	Vec2 controlPoints[8];

	ADSR_Node(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "ADSR";
		type = ADSRUI;

		controlPoints[0] = {0.0f, 0.0f}; // static 0
		controlPoints[1] = {1.0f, 0.0f}; // ctrl 0
		controlPoints[2] = {1.0f, 1.0f}; // static 1
		controlPoints[3] = {2.0f, 1.0f}; // ctrl 1
		controlPoints[4] = {2.0f, 0.8f}; // static 2
		controlPoints[5] = {3.0f, 0.8f}; // static 3
		controlPoints[6] = {3.0f, 0.0f}; // ctrl 2
		controlPoints[7] = {4.0f, 0.0f}; // static 4

		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
		inputs.push_back(createPin(idManager, "> input", PinKind::Input));
		inputs.push_back(createPin(idManager, "> trigger", PinKind::Input));
	}

	AudioComponent* convertNodeToAudioComponent() override
	{
		ADSR* audioComponent = new ADSR;
		for (int i = 0; i < 8; i++)
			audioComponent->reference.controlPoints[i] = controlPoints[i];
		return audioComponent;
	}

	void render(std::queue<Message>& messages) override
	{
		Node::startRender();
		Node::renderNameAndPins();

		ImGui::PushID(appendId("Edit ADSR").c_str());
		if (ImGui::Button("Edit ADSR"))
			messages.push(Message(UI_SHOW_ADSR_EDITOR, new MessageNodeIdAndControlPoints{controlPoints, id}));
		ImGui::PopID();
		Node::endRender();
	}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(
			cereal::make_nvp("base_node", cereal::base_class<Node>(this)),
			cereal::make_nvp("control_points", controlPoints)
		);
	}
};

struct KeyboardFrequencyNode : public Node {
	KeyboardFrequencyNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Keyboard Frequency";
		type = KbFreqUI;

		outputs.push_back(createPin(idManager, "frequency >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override { return new KeyboardFrequency; }
};

struct MultNode : public Node {
	MultNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Multiplier";
		type = MultUI;

		inputs.push_back(createPin(idManager, "> input A", PinKind::Input));
		inputs.push_back(createPin(idManager, "> input B", PinKind::Input));
		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override { return new Multiplier; }
};

struct LowPassFilterNode : public Node {
	LowPassFilterNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Low Pass Filter";
		type = LowPassUI;
		inputs.push_back(createPin(idManager, "> signal", PinKind::Input));
		inputs.push_back(createPin(idManager, "> alpha", PinKind::Input));
		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override { return new LowPassFilter; }
};

struct CombFilterNode : public Node {
	CombFilterNode(IDManager* idManager = nullptr)
	{
		id = getId(idManager);
		name = "Comb Filter";
		type = CombFilterUI;

		inputs.push_back(createPin(idManager, "> input", PinKind::Input));
		inputs.push_back(createPin(idManager, "> delay samples", PinKind::Input));
		inputs.push_back(createPin(idManager, "> feedback", PinKind::Input));
		outputs.push_back(createPin(idManager, "output >", PinKind::Output));
	}

	AudioComponent* convertNodeToAudioComponent() override { return new CombFilter; }
};

// Register every Node child classes
CEREAL_REGISTER_TYPE(MasterNode)
CEREAL_REGISTER_TYPE(NumberNode)
CEREAL_REGISTER_TYPE(OscNode)
CEREAL_REGISTER_TYPE(ADSR_Node)
CEREAL_REGISTER_TYPE(KeyboardFrequencyNode)
CEREAL_REGISTER_TYPE(MultNode)
CEREAL_REGISTER_TYPE(LowPassFilterNode)
CEREAL_REGISTER_TYPE(CombFilterNode)

// Register child class if it does not have serialization method.
// This indicate cereal to use Node serialization method.
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, MasterNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, KeyboardFrequencyNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, MultNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, LowPassFilterNode)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Node, CombFilterNode)
