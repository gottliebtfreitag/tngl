#include "Link.h"
#include "Node.h"

namespace tngl
{


LinkBase::LinkBase(Node* _owner, Flags _flags, std::string const& _regex)
	: flags(_flags)
	, regexStr(_regex)
	, owner(_owner)
{
	if (regexStr == "") {
		if (Flags::None != (flags & Flags::CreateIfNotExist)) {
			throw std::runtime_error("must not pass empty regex to conjunction if specified Flags::CreateIfNotExist");
		}
		// default to a match all in other cases
		regexStr = ".*";
	}
	regex = std::regex(regexStr);
	owner->addLink(this);
}

LinkBase::~LinkBase() {
	if (owner) {
		owner->removeLink(this);
	}
}

}
