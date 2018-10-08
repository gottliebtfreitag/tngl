#pragma once

#include <regex>
#include <cstddef>
#include <typeinfo>

namespace tngl {

struct Node;

enum class Flags : int {
	None = 0,
	CreateIfNotExist = 1,
	Required = 2,
};
constexpr Flags operator|(Flags const& l, Flags const& r) {
	return static_cast<Flags>(static_cast<int>(l) | static_cast<int>(r));
}
constexpr Flags operator&(Flags const& l, Flags const& r) {
	return static_cast<Flags>(static_cast<int>(l) & static_cast<int>(r));
}

struct LinkBase {
	LinkBase(Node* owner, Flags _flags=Flags::None, std::string const& _regex="");
	virtual ~LinkBase();

	Flags getFlags() const {
		return flags;
	}

	virtual bool canSetOther(Node const*) const = 0;
	virtual void setOther(Node*) = 0;
	virtual bool satisfied() = 0;

	virtual bool isConnectedTo(Node const*) const = 0;

	bool matchesName(std::string const& name) const {
		return std::regex_match(name, regex);
	}

	virtual std::type_info const& getType() const = 0;

	std::string const& getRegex() const {
		return regexStr;
	}
private:
	Flags flags {Flags::None};
	std::regex regex;
	std::string regexStr;
	Node* owner {nullptr};
};

template<typename T=Node>
struct Link : LinkBase {
	using LinkBase::LinkBase;

	std::type_info const& getType() const override {
		return typeid(T);
	}
	bool canSetOther(Node const* other) const override {
		return dynamic_cast<T const*>(other);
	}
	void setOther(Node* other) override {
		T* otherCast = dynamic_cast<T*>(other);
		if (otherCast) {
			node = otherCast;
		}
	}

	bool satisfied() override {
		return node;
	}

	bool isConnectedTo(Node const* other) const override {
		return other == dynamic_cast<Node const*>(node);
	}

	T      * operator->()       { return node; }
	T const* operator->() const { return node; }

	T      & operator *()       { return *node; }
	T const& operator *() const { return *node; }

	operator T      * ()        { return node; }
	operator T const* () const  { return node; }
private:
	T* node {nullptr};
};

template<typename T=Node>
struct Links : LinkBase {
private:
	std::vector<T*> nodes;
public:
	Links(Node* owner, std::string const& _regex=".*")
	: LinkBase(owner, Flags::None, _regex)
	{}
	Links(Node* owner, Flags flags, std::string const& _regex=".*")
	: LinkBase(owner, flags, _regex)
	{}

	std::type_info const& getType() const override {
		return typeid(T);
	}
	bool canSetOther(Node const* other) const override {
		return dynamic_cast<T const*>(other);
	}
	void setOther(Node* other) override {
		T* otherCast = dynamic_cast<T*>(other);
		if (otherCast) {
			nodes.emplace_back(otherCast);
		}
	}

	bool satisfied() override {
		return false;
	}

	bool isConnectedTo(Node const* other) const override {
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](T const* o) { return other == dynamic_cast<Node const*>(o);});
		return it != nodes.end();
	}

	std::vector<T*> const& getNodes() {
		return nodes;
	}

	auto begin() -> decltype(nodes.begin()) { return nodes.begin(); }
	auto end() -> decltype(nodes.end()) { return nodes.end(); }

};

}


