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
	CreateRequired = 3,
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

	virtual bool canSetOther(Node const* other) const = 0;
	virtual void setOther(Node* other, std::string const& name) = 0;
	virtual bool satisfied() const = 0;

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
private:
	T* node {nullptr};
	std::string otherName;
public:

	using LinkBase::LinkBase;

	std::type_info const& getType() const override {
		return typeid(T);
	}
	bool canSetOther(Node const* other) const override {
		return dynamic_cast<T const*>(other);
	}
	void setOther(Node* other, std::string const& name) override {
		T* otherCast = dynamic_cast<T*>(other);
		if (otherCast) {
			node = otherCast;
			otherName = name;
		}
	}

	bool satisfied() const override {
		return node;
	}

	bool isConnectedTo(Node const* other) const override {
		return other == dynamic_cast<Node const*>(node);
	}

	T      * operator->()       { return node; }
	T const* operator->() const { return node; }

	T      & operator *()       { return *node; }
	T const& operator *() const { return *node; }

	T      * get()       { return node; }
	T const* get() const { return node; }

	operator bool() const { return node != nullptr; }

	auto getOtherName() const -> decltype(otherName) const& {
		return otherName;
	}
};

template<typename T=Node>
struct Links : LinkBase {
private:
	std::map<std::string, T*> nodes;
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
	void setOther(Node* other, std::string const& name) override {
		T* otherCast = dynamic_cast<T*>(other);
		if (otherCast) {
			nodes.emplace(name, otherCast);
		}
	}

	bool satisfied() const override {
		return false;
	}

	bool isConnectedTo(Node const* other) const override {
		auto it = std::find_if(nodes.begin(), nodes.end(), [&](auto o) { return other == dynamic_cast<Node const*>(o.second);});
		return it != nodes.end();
	}

	auto getNodes() const -> decltype(nodes) const& {
		return nodes;
	}

	auto begin() -> decltype(nodes.begin()) { return nodes.begin(); }
	auto end() -> decltype(nodes.end()) { return nodes.end(); }

};

}


