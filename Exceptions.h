#pragma once

#include <stdexcept>
#include <vector>
#include <string>

#include "Link.h"

namespace tngl {

struct NodeNotCreatableError : std::runtime_error {
	std::string nodeName;
	NodeNotCreatableError(std::string const& _nodeName, std::string const& msg)
		: std::runtime_error(msg)
		, nodeName{_nodeName}
	{}
};

struct NodeLinksNotSatisfiedError : std::runtime_error {
	std::vector<LinkBase*> unsatisfiedLinks;
	Node const* node;

	NodeLinksNotSatisfiedError(std::vector<LinkBase*> links, Node const* _node, std::string const& msg)
		: std::runtime_error(msg)
		, unsatisfiedLinks(links)
		, node(_node)
	{}
};


struct NodeInitializeError : std::runtime_error {
	Node const* node;
	std::string name;

	NodeInitializeError(Node const* _node, std::string const& _name, std::string const& msg)
		: std::runtime_error(msg)
		, node(_node)
		, name(_name)
	{}
};

}


