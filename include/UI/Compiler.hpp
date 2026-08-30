#pragma once

#include <memory>

#include "AudioBackend/Components/Components.hpp"
#include "AudioBackend/Instrument.hpp"

#include "UI/NodeManager.hpp"
#include "UI/LinkManager.hpp"

#include "Logger.hpp"

class Compiler {
public:
	static void compile(Instrument& instrument, NodeManager& nodeManager, LinkManager& linkManager);

private:
	static std::vector<std::vector<std::shared_ptr<Node>>> getNodeInputs(const std::shared_ptr<Node>& node, NodeManager& nodeManager, LinkManager& linkManager);

	static void draw(const std::shared_ptr<Node>& node, NodeManager& nodeManager, LinkManager& linkManager, int depth = 0);
};
