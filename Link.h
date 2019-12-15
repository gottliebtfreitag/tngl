#pragma once

#include <regex>
#include <cstddef>
#include <typeinfo>

namespace tngl {

struct Node;

enum class Flags : int {
	Optional = 0,
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
	LinkBase(Node* owner, Flags _flags=Flags::Optional, std::string const& _regex="");
	LinkBase(LinkBase&&) noexcept;
	LinkBase& operator=(LinkBase&&) noexcept;

	virtual ~LinkBase();

	Flags getFlags() const {
		return flags;
	}

	virtual bool canSetOther(Node const* other) const = 0;
	virtual void unset(Node const* other) = 0;
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
	Flags flags {Flags::Optional};
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

	Link(Link&&) noexcept = default;
	Link& operator=(Link&&) noexcept = default;

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
	void unset(Node const* other) override {
		if (dynamic_cast<T const*>(other) == node) {
			node = nullptr;
			otherName = "";
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
	std::multimap<std::string, T*> nodes;
public:
	Links(Node* owner, std::string const& _regex=".*")
	: LinkBase(owner, Flags::Optional, _regex)
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
            // dont double insert a single instance
		    auto find = [=](auto const& p) { return p.second == otherCast; };
			auto it = std::find_if(nodes.begin(), nodes.end(), find);
			if (it == nodes.end()) {
			    nodes.emplace(name, otherCast);
            }
		}
	}
	void unset(Node const* other) override {
		T const* otherCast = dynamic_cast<T const*>(other);
		auto find = [=](auto const& p) { return p.second == otherCast; };
		while (true) {
			auto it = std::find_if(nodes.begin(), nodes.end(), find);
			if (it == nodes.end()) {
				break;
			}
			nodes.erase(it);
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
	auto end()   -> decltype(nodes.end())   { return nodes.end(); }

	auto begin() const -> decltype(nodes.cbegin()) { return nodes.cbegin(); }
	auto end()   const -> decltype(nodes.cend())   { return nodes.cend(); }

};

}


