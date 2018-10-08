#pragma once

#include "Exceptions.h"
#include "Node.h"
#include "Link.h"
#include "Factory.h"

#include <memory>
#include <set>
#include <string>


namespace tngl {

struct Tngl final {
	using ExceptionHandler = std::function<void(std::exception const&)>;
	Tngl(std::set<std::string> const& names, ExceptionHandler const& errorHandler);
	~Tngl();

	template<typename T=Node>
	T* getNodeByName(std::string const& name) const {
		return dynamic_cast<T*>(getNodeByNameImpl(name));
	}

	void initialize(ExceptionHandler const& errorHandler);
	void deinitialize();

	std::map<std::string, Node*> getNodes() const;
private:
	Node* getNodeByNameImpl(std::string const& name) const;
	struct Pimpl;
	std::unique_ptr<Pimpl> pimpl;
};

}
