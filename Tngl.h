#pragma once

#include "Node.h"

#include <set>
#include <string>
#include <memory>

namespace tngl
{

struct Tngl final {
	Tngl(std::set<std::string> const& names);
	~Tngl();

	template<typename T=Node>
	T* getNodeByName(std::string const& name) {
		return dynamic_cast<T*>(getNodeByName_(name));
	}

	void initialize();
private:
	Node* getNodeByName_(std::string const& name);
	struct Pimpl;
	std::unique_ptr<Pimpl> pimpl;
};

}
