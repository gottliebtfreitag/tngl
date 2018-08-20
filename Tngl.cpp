#include "Node.h"
#include "Tngl.h"

#include <map>
#include <iostream>
#include <stdexcept>
#include <regex>

namespace tngl
{

#include <cxxabi.h>
#include <iostream>

namespace
{

std::string demangle(std::type_info const& ti) {
	int status;
	char* name_ = abi::__cxa_demangle(ti.name(), 0, 0, &status);
	if (status) {
		throw std::runtime_error("cannot demangle a type!");
	}
	std::string demangledName{ name_ };
	free(name_); // need to use free here :/
	return demangledName;
}

}

struct Tngl::Pimpl {
	std::map<std::string, std::unique_ptr<Node>> nodes;
};


Tngl::Tngl(std::set<std::string> const& names)
	: pimpl(new Pimpl)
{
	auto const& creatables = NodeBuilder::NodeBuilderRegistry::getInstance().builders;
	std::set<std::string> to_create{names};

	auto& nodes = pimpl->nodes;
	// build all nodes
	while (not to_create.empty()) {
		auto name = *to_create.begin();
		to_create.erase(to_create.begin());
		// avoid recreation
		if (nodes.find(name) != nodes.end()) {
			continue;
		}
		std::regex match{name};
		for (auto const& creator : creatables) {
			if (std::regex_match(creator.first, match)) {
				if (nodes.find(creator.first) != nodes.end()) {
					continue;
				}
				try {
					std::cout << "creating: " << creator.first << "\n";
					auto elem = pimpl->nodes.emplace(creator.first, creator.second->create());
					for (auto c : elem.first->second->getLinks()) {
						if (Flags::None != (c->getFlags() & Flags::CreateIfNotExist)) {
							// find all matches and queue them to be created
							for (auto const& creatable : creatables) {
								if (c->matchesName(creatable.first)) {
									// if we didn't create the related element already we need to enqueue it
									if (nodes.find(creatable.first) == nodes.end()) {
										to_create.insert(creatable.first);
									}
								}
							}
						}
					}
				} catch (std::runtime_error const& err) {
					std::cerr << "exception happened while building " << name << ":\n" << err.what() << std::endl;
				}
			}
		}
	}
	// drop all nodes whose required links cannot be satisfied
	while (true) {
		std::vector<LinkBase*> unsatisfiedLinks;
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](std::pair<std::string const, std::unique_ptr<Node>> const& node) {
			bool canSatisfyAll = true;
			for (auto const& link: node.second->getLinks()) {
				if (tngl::Flags::Required == (link->getFlags() & tngl::Flags::Required)) {
					bool canSatisfy = false;
					for (auto const& otherNode : nodes) {
						if (link->canSetOther(otherNode.second.get())) {
							canSatisfy = true;
							break;
						}
					}
					if (not canSatisfy) {
						unsatisfiedLinks.push_back(link);
						canSatisfyAll = false;
					}
				}
			}
			return not canSatisfyAll;
		});
		if (it == nodes.end()) {
			break;
		}
		std::cout << "cannot create a valid environment for " << it->first << "\n";
		std::cout << "the following links cannot be satisfied: \n";
		for (auto link : unsatisfiedLinks) {
			std::cout << "  " << demangle(link->getType()) << "\n";
		}
		std::cout << std::flush;
		nodes.erase(it);
	}

	// associate all links
	for (auto& node : nodes) {
		for (auto link : node.second->getLinks()) {
			for (auto& other_node : nodes) {
				if (link->matchesName(other_node.first)) {
					link->setOther(other_node.second.get());
				}
				if (link->satisfied()) {
					break;
				}
			}
			if (not link->satisfied() and Flags::None != (link->getFlags() & Flags::Required)) {
				throw std::runtime_error("cannot satisfy required conjunction of " + node.first);
			}
		}
	}

	// call initialize on each node
	for (auto& b : nodes) {
		std::cout << "init: " << b.first << "\n";
		b.second->initialize();
	}

}

Tngl::~Tngl() {
	// call deinitialize on each node
	for (auto& b : pimpl->nodes) {
		std::cout << "deinit: " << b.first << "\n";
		b.second->deinitialize();
	}
}

Node* Tngl::getNodeByName_(std::string const& name) {
	auto it = pimpl->nodes.find(name);
	if (it != pimpl->nodes.end()) {
		auto val = it->second.get();
		return val;
	}
	return nullptr;
}

std::set<std::string> Tngl::getAllNodeTypeNames() {
	return {};
}

}


