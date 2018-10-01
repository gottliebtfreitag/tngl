#pragma once

#include "Factory.h"

#include "Link.h"

#include <algorithm>
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

	virtual void initializeNode() {};
	virtual void deinitializeNode() noexcept {};

	auto getLinks() const -> decltype(links) const& {
		return links;
	}

	void addLink(LinkBase* link) {
		links.emplace_back(link);
	}
	void removeLink(LinkBase* link) {
		links.erase(std::remove(links.begin(), links.end(), link), links.end());
	}
};

}
