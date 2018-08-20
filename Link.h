#pragma once

#include "Node.h"

#include <regex>
#include <cstddef>
#include <typeinfo>

namespace tngl
{
struct Node;

enum class Flags : int {
	None = 0,
	CreateIfNotExist=1,
	Required=2
};
constexpr Flags operator|(Flags const& l, Flags const& r) {
	return static_cast<Flags>(static_cast<int>(l) | static_cast<int>(r));
}
constexpr Flags operator&(Flags const& l, Flags const& r) {
	return static_cast<Flags>(static_cast<int>(l) & static_cast<int>(r));
}

struct LinkBase {
	LinkBase(Node* owner, Flags _flags=Flags::None, std::string const& _regex="");
	virtual ~LinkBase() = default;

	Flags getFlags() const {
		return flags;
	}

	virtual bool canSetOther(Node const*) const = 0;
	virtual void setOther(Node*) = 0;
	virtual bool satisfied() = 0;

	bool matchesName(std::string const& name) const {
		return std::regex_match(name, regex);
	}

	virtual std::type_info const& getType() const = 0;

private:
	Flags flags;
	std::regex regex;
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
			block = otherCast;
		}
	}

	bool satisfied() override {
		return block;
	}

	T      * operator->()       { return block; }
	T const* operator->() const { return block; }

	T      & operator *()       { return *block; }
	T const& operator *() const { return *block; }

	operator T      * ()        { return block; }
	operator T const* () const  { return block; }
private:
	T* block {nullptr};
};

template<typename T=Node>
struct Links : LinkBase {
private:
	std::vector<T*> blocks;
public:
	Links(Node* owner, std::string const& _regex=".*")
	: LinkBase(owner, Flags::None, _regex)
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
			blocks.emplace_back(otherCast);
		}
	}

	bool satisfied() override {
		return false;
	}

	std::vector<T*> const& getBlocks() {
		return blocks;
	}

	auto begin() -> decltype(blocks.begin()) { return blocks.begin(); }
	auto end() -> decltype(blocks.end()) { return blocks.end(); }

};

}


