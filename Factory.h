#pragma once

#include "Singleton.h"
#include "Node.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

namespace tngl {

struct NodeBuilderBase;

using NodeBuilders = std::multimap<std::string, NodeBuilderBase const*>;
using NodeBuilderRegistry = Singleton<NodeBuilders>;

struct NodeBuilderBase {

private:
    std::string _name;
    std::type_info const& _info;
    std::function<std::unique_ptr<Node>()> _createFunc;

public:
    template<typename Func>
    NodeBuilderBase(std::string name, std::type_info const& info, Func f)
        : _name{std::move(name)}
        , _info{info}
        , _createFunc{[=] { return std::unique_ptr<Node>{f()}; }} {
        NodeBuilderRegistry::getInstance().emplace(_name, this);
    }

    NodeBuilderBase(NodeBuilderBase const&) = delete;
    NodeBuilderBase& operator=(NodeBuilderBase const&) = delete;
    ~NodeBuilderBase() {
        NodeBuilderRegistry::getInstance().erase(_name);
    }

    std::unique_ptr<Node> create() const {
        return _createFunc();
    }

    std::type_info const& getType() const {
        return _info;
    }
};

template<typename T>
struct NodeBuilder : NodeBuilderBase {
    using NodeBuilderBase::NodeBuilderBase;

    NodeBuilder(std::string const& name)
        : NodeBuilderBase(name, typeid(T), []{
            if constexpr (std::is_default_constructible_v<T>) {
                return std::make_unique<T>();
            } else {
                return nullptr;
            }
        })
    {}

    template <typename Func>
    NodeBuilder(std::string const& name, Func f)
        : NodeBuilderBase(name, typeid(T), f)
    {}
};

namespace detail {
bool is_type_ancestor(const std::type_info& base, const std::type_info& deriv);
}

using Builders = NodeBuilderRegistry::value_type;
Builders getBuildersForType(const std::type_info& base);

// get all builders that can produce a specialization of T or a T itself
template<typename T>
Builders getBuildersForType() {
    return getBuildersForType(typeid(T));
}

}
