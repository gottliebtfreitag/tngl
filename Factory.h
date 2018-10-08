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
	std::function<std::unique_ptr<Node>()> _createFunc;
	std::string _name;

	void registerThis() noexcept {
		NodeBuilderRegistry::getInstance().builders.emplace(_name, this);
	}
	void unregisterThis() noexcept {
		auto& builders = NodeBuilderRegistry::getInstance().builders;
		auto it = builders.find(_name);
		if (it != builders.end() and it->second == this) {
			builders.erase(it);
		}
	}
public:
	template<typename Func>
	NodeBuilderBase(std::string const& name, Func f)
		: _createFunc{[=]{ return std::unique_ptr<Node>{f()};}}
		, _name{name} {
		registerThis();
	}

	NodeBuilderBase& operator=(NodeBuilderBase && other) noexcept {
		unregisterThis();
		other.unregisterThis();
		std::swap(_createFunc, other._createFunc);
		std::swap(_name, other._name);
		registerThis();
		return *this;
	}
	NodeBuilderBase(NodeBuilderBase && other) noexcept {
		*this = std::move(other);
	}
	~NodeBuilderBase() {
		unregisterThis();
	}

	std::unique_ptr<Node> create() {
		return _createFunc();
	}

	virtual std::type_info const& getType() const = 0;
};

template<typename T>
struct NodeBuilder : NodeBuilderBase {
	using NodeBuilderBase::NodeBuilderBase;

	NodeBuilder(NodeBuilder && other) noexcept
		: NodeBuilderBase(std::move(other))
	{}
	NodeBuilder& operator=(NodeBuilder && other) noexcept {
		NodeBuilderBase::operator =(std::move(other));
		return *this;
	}

	NodeBuilder(std::string const& name) : NodeBuilderBase(name, []{return std::make_unique<T>();}) {}

	virtual ~NodeBuilder() = default;
	std::type_info const& getType() const override {
		return typeid(T);
	}

};

}
