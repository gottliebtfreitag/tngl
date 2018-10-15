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
	T* getNode(std::regex const& regex) const {
		for (auto node : getNodesImpl(regex)) {
			T* cast = dynamic_cast<T*>(node);
			if (cast) {
				return cast;
			}
		}
		return nullptr;
	}

	template<typename T=Node>
	T* getNode(std::string const& regex=".*") const {
		return getNode<T>(std::regex(regex));
	}

	void initialize(ExceptionHandler const& errorHandler);
	void deinitialize();

	std::map<std::string, Node*> getNodes() const;
private:
	std::vector<Node*> getNodesImpl(std::regex const& regex) const;
	struct Pimpl;
	std::unique_ptr<Pimpl> pimpl;
};

}
