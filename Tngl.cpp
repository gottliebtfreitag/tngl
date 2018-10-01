#include "Node.h"
#include "Tngl.h"

#include <map>
#include <iostream>
#include <stdexcept>
#include <regex>

#include <cxxabi.h>
#include <iostream>

namespace tngl
{


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


Tngl::Tngl(std::set<std::string> const& names)
	: pimpl(new Pimpl)
{
	auto const& creatables = NodeBuilderBase::NodeBuilderRegistry::getInstance().builders;

	std::set<std::string> to_create_queue;
	for (auto const& name : names) {
		std::regex regex{name};
		for (auto const& creatable : creatables) {
			if (std::regex_match(name, regex)) {
				to_create_queue.insert(creatable.first);
			}
		}
	}

	auto& nodes = pimpl->nodes;
	// build all nodes
	while (not to_create_queue.empty()) {
		auto to_create = *to_create_queue.begin();
		to_create_queue.erase(to_create_queue.begin());
		// avoid recreation
		if (nodes.find(to_create) != nodes.end()) {
			continue;
		}
		auto it = creatables.find(to_create);
		if (it == creatables.end()) {
			std::cerr << "there is no creator for Node with name " + to_create;
		}
		// build the new node
		std::unique_ptr<Node> newNode = it->second->create();

		// add all links which are not created yet to the create queue
		for (auto const link : newNode->getLinks()) {
			if (Flags::None != (link->getFlags() & Flags::CreateIfNotExist)) {
				for (auto const& creatable : creatables) {
					if (link->matchesName(creatable.first)) {
						// if we didn't create the related element already we need to enqueue it
						if (nodes.find(creatable.first) == nodes.end() and is_ancestor(link->getType(), creatable.second->getType())) {
							to_create_queue.insert(creatable.first);
						}
					}
				}
			}
		}
		pimpl->nodes.emplace(to_create, std::move(newNode));
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
}

Tngl::~Tngl() {}

Node* Tngl::getNodeByName_(std::string const& name) {
	auto it = pimpl->nodes.find(name);
	if (it != pimpl->nodes.end()) {
		auto val = it->second.get();
		return val;
	}
	return nullptr;
}

void Tngl::initialize() {
	for (auto& b : pimpl->nodes) {
		std::cout << "init: " << b.first << "\n";
		b.second->initializeNode();
	}
}

void Tngl::deinitialize() {
	for (auto& b : pimpl->nodes) {
		std::cout << "init: " << b.first << "\n";
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
