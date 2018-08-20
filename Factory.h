#pragma once

#include "Registry.h"

#include <functional>
#include <string>
#include <memory>
#include <map>

namespace tngl {

struct Node;
struct NodeBuilder;

struct NodeBuilder {
	struct NodeBuilderRegistry {
		std::map<std::string, NodeBuilder*> builders;
		static NodeBuilderRegistry& getInstance() {
			static NodeBuilderRegistry instance;
			return instance;
		}
	};
private:
	std::function<std::unique_ptr<Node>()> _createFunc;
	std::string _name;
public:
	template<typename Func>
	NodeBuilder(std::string const& name, Func f)
		: _createFunc{[=]{ return std::unique_ptr<Node>{f()};}}
		, _name{name}
	{
		NodeBuilderRegistry::getInstance().builders.emplace(_name, this);
	}

	~NodeBuilder() {
		auto& builders = NodeBuilderRegistry::getInstance().builders;
		auto it = builders.find(_name);
		if (it != builders.end()) {
			builders.erase(it);
		}
	}

	std::unique_ptr<Node> create() {
		return _createFunc();
	}
};

template<typename T>
NodeBuilder simpleBuilder(std::string const& name) {
	return NodeBuilder(name, []{return std::make_unique<T>();});
}


}
