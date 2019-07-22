#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace tngl {

struct LinkBase;

struct Node {
private:
	std::vector<LinkBase*> links;

public:
	virtual ~Node() = default;

	virtual void initializeNode() {};
	virtual void startNode() {};
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
