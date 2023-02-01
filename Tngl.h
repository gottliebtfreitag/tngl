#pragma once

#include "Exceptions.h"
#include "Factory.h"
#include "Link.h"
#include "Node.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace tngl {

struct Tngl final {
    using ExceptionHandler = std::function<void(std::exception const&)>;
    Tngl(Node& seedNode, std::string const& seedNodeName, ExceptionHandler const& errorHandler, NodeBuilders const& nodeBuilders = NodeBuilderRegistry::getInstance());
    Tngl(std::map<std::string, Node*> const& seedNodes, ExceptionHandler const& errorHandler, NodeBuilders const& nodeBuilders = NodeBuilderRegistry::getInstance());
    ~Tngl();

    template <typename T = Node>
    std::pair<std::string, T*> getNode(std::regex const& regex) const
    {
        for (auto const& node : getNodesImpl(regex)) {
            T* cast = dynamic_cast<T*>(node.second);
            if (cast) {
                return { node.first, cast };
            }
        }
        return { "", nullptr };
    }

    template <typename T = Node>
    std::multimap<std::string, T*> getNodes(std::regex const& regex) const
    {
        std::multimap<std::string, T*> nodes;
        for (auto const& node : getNodesImpl(regex)) {
            T* cast = dynamic_cast<T*>(node.second);
            if (cast) {
                nodes.emplace(node.first, cast);
            }
        }
        return nodes;
    }

    template <typename T = Node>
    std::pair<std::string, T*> getNode(std::string const& regex = ".*") const
    {
        return getNode<T>(std::regex(regex));
    }

    template <typename T = Node>
    std::multimap<std::string, T*> getNodes(std::string const& regex = ".*") const
    {
        return getNodes<T>(std::regex(regex));
    }

    void initialize(ExceptionHandler const& errorHandler);
    void deinitialize();

    std::multimap<std::string, Node*> getNodes() const;

private:
    std::multimap<std::string, Node*> getNodesImpl(std::regex const& regex) const;
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}
