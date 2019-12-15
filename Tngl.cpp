#include "Tngl.h"

#include <map>
#include <iostream>
#include <stdexcept>
#include <regex>
#include <iostream>

namespace tngl {

struct Tngl::Pimpl {
    std::vector<Node*> seedNodes;
	std::multimap<std::string, std::unique_ptr<Node>> nodes;
};


Tngl::Tngl(Node& seedNode, ExceptionHandler const& errorHandler)
    : Tngl({&seedNode}, errorHandler)
{}
Tngl::Tngl(std::vector<Node*> const& seedNodes, ExceptionHandler const& errorHandler) 
	: pimpl(new Pimpl)
{
	auto& nodes = pimpl->nodes;
    auto const& creators = NodeBuilderRegistry::getInstance();

    pimpl->seedNodes = seedNodes;

    // hook the seed nodes together
    for (auto& sn : pimpl->seedNodes) {
        for (auto& link : sn->getLinks()) {
            if (link->satisfied()) {
                continue;
            }
            for (auto& peer : pimpl->seedNodes) {
                if (link->matchesName("")) {
                    link->setOther(peer, "");
                    if (link->satisfied()) {
                        break;
                    }
                }
            }
        }
    }

	std::vector<LinkBase*> links;
	auto isUnsatisfied = [](LinkBase const* link) {
		return not link->satisfied() and (link->getFlags() & Flags::CreateIfNotExist) == Flags::CreateIfNotExist;
	};
    for (auto const* seedNode : pimpl->seedNodes) {
        std::copy(begin(seedNode->getLinks()), end(seedNode->getLinks()), std::back_inserter(links));
    };
	std::set<std::string> brokenCreators;

	auto findCreatorForLink = [&](LinkBase const* link) {
		return std::find_if(creators.begin(), creators.end(), [&](auto const& creator) {
			return  link->matchesName(creator.first) and // if the name matches
					brokenCreators.find(creator.first) == brokenCreators.end() and // if we did not try to create it before
					nodes.find(creator.first) == nodes.end() and // if it is not created yet
                    detail::is_type_ancestor(link->getType(), creator.second->getType()); // if it produces the right type
		});
	};

	while (true) {
		// find a link that can be set
		auto linkIt = std::find_if(links.begin(), links.end(), [&](LinkBase const* link) {
			return isUnsatisfied(link) and findCreatorForLink(link) != creators.end();
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
			for (auto& node : pimpl->seedNodes) {
				if (link->matchesName("")) {
					link->setOther(node, "");
                    if (link->satisfied()) {
                        break;
                    }
				}
			}
            if (link->satisfied()) {
                break;
            }
			for (auto& node : nodes) {
				if (link->matchesName(node.first)) {
					link->setOther(node.second.get(), node.first);
                    if (link->satisfied()) {
                        break;
                    }
				}
			}
		}
		// put all new links into the set of all links
		std::copy(begin(newNode->getLinks()), end(newNode->getLinks()), std::back_inserter(links));
		nodes.emplace(creatorIt->first, std::move(newNode));
	}

    auto isUnsatisfiedButRequired = [](LinkBase const* link) {
		return not link->satisfied() and (link->getFlags() & Flags::Required) == Flags::Required;
	};
	// drop all nodes whose required links cannot be satisfied
	auto handleBadNode = [&](Node &node, std::string const& name) {
		if (errorHandler) {
			// find all unsatisfied links and report them to the handler
			std::vector<LinkBase*>unsatisfiedLinks;
			auto const& links = node.getLinks();
			std::copy_if(links.begin(), links.end(), std::back_inserter(unsatisfiedLinks), isUnsatisfiedButRequired);
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
			return std::find_if(links.begin(), links.end(), isUnsatisfiedButRequired) != links.end();
		});
		if (it == nodes.end()) {
			break;
		}
		handleBadNode(*it->second.get(), it->first);
		nodes.erase(it);
	}
	// test if the requires of the seed note are satisfied
	{
        for (auto* seedNode : pimpl->seedNodes) {
            auto const& seedLinks = seedNode->getLinks();
            if (seedLinks.end() != std::find_if(seedLinks.begin(), seedLinks.end(), isUnsatisfiedButRequired)) {
                handleBadNode(*seedNode, "seedNode");
            }
        }
    }
}

Tngl::~Tngl() {}

void Tngl::initialize(ExceptionHandler const& errorHandler) {
    auto initializer = [&](std::string const& name, Node* node) {
		try {
			std::cout << "init: " << name << "\n";
			node->initializeNode();
		} catch (...) {
			try {
				std::throw_with_nested(NodeInitializeError{node, name, "\"" + name + "\" threw during initialization"});
			} catch (std::exception const& error) {
				if (errorHandler) {
					errorHandler(error);
				}
			}
		}
    };
    auto starter = [&](std::string const& name, Node* node) {
		try {
			node->startNode();
		} catch (...) {
			try {
				std::throw_with_nested(NodeInitializeError{node, name, "\"" + name + "\" threw during start"});
			} catch (std::exception const& error) {
				if (errorHandler) {
					errorHandler(error);
				}
			}
		}
    };
	for (auto& node : pimpl->seedNodes) {
        initializer("seed_node", node);
    }
	for (auto& [name, node] : pimpl->nodes) {
        initializer(name, node.get());
	}

	for (auto& node : pimpl->seedNodes) {
        starter("seed_node", node);
    }
	for (auto& [name, node] : pimpl->nodes) {
        starter(name, node.get());
	}
}

void Tngl::deinitialize() {
	for (auto& b : pimpl->seedNodes) {
		std::cout << "deinit: seed_node\n";
		b->deinitializeNode();
	}
	for (auto& b : pimpl->nodes) {
		std::cout << "deinit: " << b.first << "\n";
		b.second->deinitializeNode();
	}
}


std::multimap<std::string, Node*> Tngl::getNodesImpl(std::regex const& regex) const {
	std::multimap<std::string, Node*> nodes;
	for (auto node : pimpl->seedNodes) {
		if (std::regex_match("", regex)) {
			nodes.emplace("seed_node", node);
		}
	}
	for (auto &node : pimpl->nodes) {
		if (std::regex_match(node.first, regex)) {
			nodes.emplace(node.first, node.second.get());
		}
	}
	return nodes;
}

std::multimap<std::string, Node*> Tngl::getNodes() const {
	std::multimap<std::string, Node*> nodes;
	for (auto& b : pimpl->seedNodes) {
		nodes.emplace("seed_node", b);
	}
	for (auto& b : pimpl->nodes) {
		nodes.emplace(b.first, b.second.get());
	}
	return nodes;
}

}
