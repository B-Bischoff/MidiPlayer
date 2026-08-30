#include "UI/Compiler.hpp"

void Compiler::compile(Instrument& instrument, NodeManager& nodeManager, LinkManager& linkManager)
{
	const std::shared_ptr<Node>& master = nodeManager.getMasterNode();

	draw(master, nodeManager, linkManager);
}

void Compiler::draw(const std::shared_ptr<Node>& node, NodeManager& nodeManager, LinkManager& linkManager, int depth)
{
	const auto& inputs = getNodeInputs(node, nodeManager, linkManager);

	unsigned int inputIndex = 0;
	for (const auto& inputNodes : inputs)
	{
		for (const std::shared_ptr<Node>& inputNode : inputNodes)
		{
			for (int i = 0; i < depth; i++)
				std::cout << "    ";
			Logger::log("Compiler", Info) << inputIndex << " Compiling node: " << inputNode->name << " id " << inputNode->id << std::endl;
			draw(inputNode, nodeManager, linkManager, depth + 1);
		}
		inputIndex++;
	}
}

std::vector<std::vector<std::shared_ptr<Node>>> Compiler::getNodeInputs(const std::shared_ptr<Node>& node, NodeManager& nodeManager, LinkManager& linkManager)
{
	std::vector<std::vector<std::shared_ptr<Node>>> inputs;

	for (Pin& pin : node->inputs)
	{
		std::list<LinkInfo> links = linkManager.findPinLinks(pin.id, 1);
		std::vector<std::shared_ptr<Node>> inputNodes;

		for (LinkInfo& link : links)
		{
			std::shared_ptr<Node>& inputNode = nodeManager.findNodeByPinId(link.OutputId);
			inputNodes.push_back(inputNode);
		}

		inputs.push_back(inputNodes);
	}

	return inputs;
}
