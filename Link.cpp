#include "Link.h"
#include "Node.h"

namespace tngl {

LinkBase::LinkBase(Node* _owner, Flags _flags, std::string const& _regex)
	: flags(_flags)
	, regexStr(_regex)
	, owner(_owner)
{
	if (regexStr == "") {
		// default to a match all in other cases
		regexStr = ".*";
	}
	regex = std::regex(regexStr);
	owner->addLink(this);
}

LinkBase::LinkBase(LinkBase&& other) noexcept {
	*this = std::move(other);
}

LinkBase& LinkBase::operator=(LinkBase&& other) noexcept
{
	if (owner) {
		owner->removeLink(this);
	}
    
	owner    = other.owner;
	flags    = other.flags;
	regexStr = std::move(other.regexStr);
	regex    = std::move(other.regex);

    if (owner) {
	    owner->addLink(this);
		other.owner->removeLink(&other);
        other.owner = nullptr;
	}
	return *this;
}

LinkBase::~LinkBase() {
	if (owner) {
		owner->removeLink(this);
	}
}

}
