#pragma once

#include "Node.h"

#include <set>
#include <string>
#include <memory>

#include "Exceptions.h"

namespace tngl
{

struct Tngl final {
	using ExceptionHandler = std::function<void(std::exception const&)>;
	Tngl(std::set<std::string> const& names, ExceptionHandler const& errorHandler);
	~Tngl();

	template<typename T=Node>
	T* getNodeByName(std::string const& name) {
		return dynamic_cast<T*>(getNodeByName_(name));
	}

	void initialize(ExceptionHandler const& errorHandler);
	void deinitialize();

	std::map<std::string, Node*> getNodes() const;
private:
	Node* getNodeByName_(std::string const& name);
	struct Pimpl;
	std::unique_ptr<Pimpl> pimpl;
};

}
