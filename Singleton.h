#pragma once

namespace tngl {

template<typename T>
struct Singleton {
    using value_type = T;

	static value_type& getInstance() {
		static value_type instance;
		return instance;
	}
protected:
	Singleton() = default;
	~Singleton() = default;

	Singleton(Singleton const&) = delete;
	Singleton(Singleton &&) = delete;
	Singleton& operator=(Singleton const&) = delete;
	Singleton& operator=(Singleton &&) = delete;
};

}
