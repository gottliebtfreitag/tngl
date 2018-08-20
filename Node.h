#pragma once

#include "Factory.h"

#include "Link.h"

#include <vector>
#include <memory>
#include <string>

namespace tngl
{

struct LinkBase;

struct Node {
private:
	std::vector<LinkBase*> links;
public:

	virtual ~Node() = default;

	virtual void initialize() {};
	virtual void deinitialize() noexcept {};

	auto getLinks() const -> decltype(links) {
		return links;
	}

	void addLink(LinkBase* link) {
		links.emplace_back(link);
	}
};

}
