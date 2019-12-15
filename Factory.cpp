#include "Factory.h"
#include <cxxabi.h>

namespace tngl {
namespace detail {

namespace {

bool walk_tree(const __cxxabiv1:: __si_class_type_info *si, std::type_info const& a) {
	if (si->__base_type == &a) {
		return true;
	}
	return is_type_ancestor(a, *si->__base_type);
}

bool walk_tree(__cxxabiv1::__vmi_class_type_info const* mi, std::type_info const& a) {
	for (unsigned int i = 0; i < mi->__base_count; ++i) {
		if (is_type_ancestor(a, *mi->__base_info[i].__base_type)) {
			return true;
		}
	}
	return false;
}
}


bool is_type_ancestor(const std::type_info& base, const std::type_info& deriv) {
	if (base == deriv) {
		return true;
	}
	const __cxxabiv1:: __si_class_type_info *si = dynamic_cast<__cxxabiv1::__si_class_type_info const*>(&deriv);
	if (si) {
		return walk_tree(si, base);
	}
	const __cxxabiv1:: __vmi_class_type_info *mi = dynamic_cast<__cxxabiv1::__vmi_class_type_info const*>(&deriv);
	if (mi) {
		return walk_tree(mi, base);
	}
	return false;
}
}

Builders getBuildersForType(const std::type_info& base) {
    auto const& reg = NodeBuilderRegistry::getInstance();
    Builders builders;
    std::for_each(begin(reg), end(reg), [&](auto const& pair) {
        if (detail::is_type_ancestor(base, pair.second->getType())) {
            builders.emplace(pair.first, pair.second);
        }
    });
    return builders;
}

}
