#pragma once

namespace tngl {

template<typename T>
struct Singleton {
	static T& getInstance() {
		static T instance;
		return instance;
	}
protected:
	Singleton() = default;
	~Singleton() = default;

	Singleton(Singleton const&) = delete;
};

}
