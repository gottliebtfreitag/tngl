#include "Node.h"
#include "Tngl.h"

#include <map>
#include <iostream>
#include <stdexcept>
#include <regex>

#include <cxxabi.h>
#include <iostream>

namespace tngl {

namespace {

bool is_ancestor(std::type_info const& a, std::type_info const& b);
bool walk_tree(const __cxxabiv1:: __si_class_type_info *si, std::type_info const& a) {
	if (si->__base_type == &a) {
		return true;
	}
	return is_ancestor(a, *si->__base_type);
}

bool walk_tree(__cxxabiv1::__vmi_class_type_info const* mi, std::type_info const& a) {
	for (unsigned int i = 0; i < mi->__base_count; ++i) {
		if (is_ancestor(a, *mi->__base_info[i].__base_type)) {
			return true;
		}
	}
	return false;
}

bool is_ancestor(const std::type_info& a, const std::type_info& b) {
	if (a == b) {
		return true;
	}
	const __cxxabiv1:: __si_class_type_info *si = dynamic_cast<__cxxabiv1::__si_class_type_info const*>(&b);
	if (si) {
		return walk_tree(si, a);
	}
	const __cxxabiv1:: __vmi_class_type_info *mi = dynamic_cast<__cxxabiv1::__vmi_class_type_info const*>(&b);
	if (mi) {
		return walk_tree(mi, a);
	}
	return false;
}

}

struct Tngl::Pimpl {
	std::map<std::string, std::unique_ptr<Node>> nodes;
};

Tngl::Tngl(Node& seedNode, ExceptionHandler const& errorHandler)
	: pimpl(new Pimpl)
{
	auto const& creators = NodeBuilderRegistry::getInstance();
	auto& nodes = pimpl->nodes;

	std::vector<LinkBase*> links = seedNode.getLinks();
	std::set<std::string> brokenCreators;

	auto findCreatorForLink = [&](LinkBase const* link) {
		return std::find_if(creators.begin(), creators.end(), [&](auto const& creator) {
			return  (link->getFlags() & Flags::CreateIfNotExist) == Flags::CreateIfNotExist and
					link->matchesName(creator.first) and // if the name matches
					brokenCreators.find(creator.first) == brokenCreators.end() and // if we did not try to create it before
					nodes.find(creator.first) == nodes.end() and // if it is not created yet
					is_ancestor(link->getType(), creator.second->getType()); // if the type matches
		});
	};

	while (true) {
		// find a link that can be set
		auto linkIt = std::find_if(links.begin(), links.end(), [&](LinkBase const* link) {
			return not link->satisfied() and
					findCreatorForLink(link) != creators.end();
		});

		if (linkIt == links.end()) {
			break; // we've exhausted the things to create
		}
		auto creatorIt = findCreatorForLink(*linkIt);
		if (creatorIt == creators.end()) {
			// if at one point we decided to build a node but cannot do so now something is terribly broken
			throw std::runtime_error("fatal internal error");
		}
		// build the new node
		std::unique_ptr<Node> newNode;
		try {
			newNode = creatorIt->second->create();
			if (not newNode) {
				throw std::runtime_error("cannot create node with name: \"" + creatorIt->first + "\"");
			}
		} catch (...) {
			brokenCreators.insert(creatorIt->first);
			try {
				std::throw_with_nested(NodeNotCreatableError{creatorIt->first, "cannot create: \"" + creatorIt->first + "\""});
			} catch (std::exception const& error) {
				if (errorHandler) {
					errorHandler(error);
				}
			}
			continue;
		}
		// set the link to the newly built node

		// put all new links into the set of all links
		std::copy(newNode->getLinks().begin(), newNode->getLinks().end(), std::back_inserter(links));

		// apply newNode to as many links as possible
		for (auto link : links) {
			if (not link->satisfied() and link->matchesName(creatorIt->first)) {
				link->setOther(newNode.get(), creatorIt->first);
			}
		}
		// set the links of newNode to everything we have created so far
		for (auto link : newNode->getLinks()) {
			for (auto& node : nodes) {
				if (link->matchesName(node.first)) {
					link->setOther(node.second.get(), node.first);
				}
			}
		}
		nodes.emplace(creatorIt->first, std::move(newNode));
	}

	// drop all nodes whose required links cannot be satisfied
	auto isUnsatisfied = [](auto link) {
		return not link->satisfied() and (link->getFlags() & Flags::Required) == Flags::Required;
	};
	auto handleBadNode = [&](Node &node, std::string const& name) {
		if (errorHandler) {
			// find all unsatisfied links and report them to the handler
			std::vector<LinkBase*>unsatisfiedLinks;
			auto const& links = node.getLinks();
			std::copy_if(links.begin(), links.end(), std::back_inserter(unsatisfiedLinks), isUnsatisfied);
			errorHandler(NodeLinksNotSatisfiedError{std::move(unsatisfiedLinks), &node, "cannot create a valid environment for " + name});
		}
		// unlink all links to it
		std::for_each(nodes.begin(), nodes.end(), [&](auto& otherNode) {
			auto const& links = otherNode.second->getLinks();
			std::for_each(links.begin(), links.end(), [&](auto link) {link->unset(&node);});
		});
	};

	while (true) {
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](auto const& node) {
			auto const& links = node.second->getLinks();
			return std::find_if(links.begin(), links.end(), isUnsatisfied) != links.end();
		});
		if (it == nodes.end()) {
			break;
		}
		handleBadNode(*it->second.get(), it->first);
		nodes.erase(it);
	}
	// test if the requires of the seed note are satisfied
	{
		auto const& seedLinks = seedNode.getLinks();
		if (seedLinks.end() != std::find_if(seedLinks.begin(), seedLinks.end(), isUnsatisfied)) {
			handleBadNode(seedNode, "seedNode");
		}
	}
}

Tngl::~Tngl() {}

std::map<std::string, Node*> Tngl::getNodesImpl(std::regex const& regex) const {
	std::map<std::string, Node*> nodes;
	for (auto &node : pimpl->nodes) {
		if (std::regex_match(node.first, regex)) {
			nodes.emplace(node.first, node.second.get());
		}
	}
	return nodes;
}

void Tngl::initialize(ExceptionHandler const& errorHandler) {
	for (auto& b : pimpl->nodes) {
		try {
			std::cout << "init: " << b.first << "\n";
			b.second->initializeNode();
		} catch (...) {
			try {
				std::throw_with_nested(NodeInitializeError{b.second.get(), b.first, "\"" + b.first + "\" threw during initialization"});
			} catch (std::exception const& error) {
				if (errorHandler) {
					errorHandler(error);
				}
			}
		}
	}

	for (auto& b : pimpl->nodes) {
		try {
			b.second->startNode();
		} catch (...) {
			try {
				std::throw_with_nested(NodeInitializeError{b.second.get(), b.first, "\"" + b.first + "\" threw during initialization"});
			} catch (std::exception const& error) {
				if (errorHandler) {
					errorHandler(error);
				}
			}
		}
	}
}

void Tngl::deinitialize() {
	for (auto& b : pimpl->nodes) {
		std::cout << "deinit: " << b.first << "\n";
		b.second->deinitializeNode();
	}
}


std::map<std::string, Node*> Tngl::getNodes() const {
	std::map<std::string, Node*> nodes;
	for (auto& b : pimpl->nodes) {
		nodes[b.first] = (b.second.get());
	}
	return nodes;
}

}
