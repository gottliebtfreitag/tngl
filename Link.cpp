#include "Link.h"
#include "Node.h"

namespace tngl
{


LinkBase::LinkBase(Node* owner, Flags _flags, std::string const& _regex)
	: flags(_flags)
	, regex(_regex)
{
	if (_regex == "") {
		if (Flags::None != (flags & Flags::CreateIfNotExist)) {
			throw std::runtime_error("must not pass empty regex to conjunction if specified Flags::CreateIfNotExist");
		}
		// default to a match all in other cases
		regex = std::regex(".*");
	}
	owner->addLink(this);
}

}
