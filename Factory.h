#pragma once

#include "Registry.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace tngl {

struct Node;
struct NodeBuilderBase {
	struct NodeBuilderRegistry {
		std::map<std::string, NodeBuilderBase*> builders;
		static NodeBuilderRegistry& getInstance() {
			static NodeBuilderRegistry instance;
			return instance;
		}
	};
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
		NodeBuilderRegistry::getInstance().builders.emplace(_name, this);
	}

	NodeBuilderBase(NodeBuilderBase const&) = delete;
	NodeBuilderBase& operator=(NodeBuilderBase const&) = delete;
	~NodeBuilderBase() {
		NodeBuilderRegistry::getInstance().builders.erase(_name);
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
			return std::make_unique<T>();
		})
	{}

	template <typename Func>
	NodeBuilder(std::string const& name, Func f)
		: NodeBuilderBase(name, typeid(T), f)
	{}
};

}
