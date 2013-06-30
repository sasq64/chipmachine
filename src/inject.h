#ifndef INJECTION_H
#define INJECTION_H

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <assert.h>
#include <stdio.h>
/*
template <typename T> class InjectionPoint {
public:
	bool apply(T &arg) {
	};
};
*/

class Inject;

class Injection {
public:
	virtual void removeListener(void *i) = 0;
};

template <typename T> class TypedInjection : public Injection {
public:
	TypedInjection(const std::string &name) {
	}

	bool apply(T &t) {
		for(auto &i : injectors) {
			bool rc = i.first(t);
			if(!rc)
				return false;
		}
		return true;
	}

	void addListener(const std::function<bool(T&)> &fn, void *ptr) {
		injectors.push_back(make_pair(fn, ptr));
	}
	void removeListener(void *ptr) {

		for(auto i : injectors) {
			if(i.second == ptr) {
				//injectors.erase(i);
			}
		}

	}

	std::vector<std::pair<std::function<bool(T&)>, void*>> injectors;

};

/*class InjectionManager {
public:

	template <typename T> void addInjectionPoint(const std::string &name) {
		injections.push_back(new TypedInjection<T>(name));
	}


}; */

extern std::unordered_map<std::string, Injection*> injections;



//void inject(const std::string &name, std::function<bool(InjectionData&)> fn) {

class Inject {
public:
	Inject() : injection(nullptr) {};
	template <typename T> Inject(const std::string &name, std::function<bool(T&)> fn) {
		injection = inject(name, fn, this);
	}

	template <typename T> Inject(const std::string &name, std::function<void(T&)> fn) {
	}

	~Inject() {
		//injection->removeListener(this);
	}

	Injection *injection;
};


template <typename T> Injection *inject(const std::string &name, std::function<bool(T&)> fn, void *i) {	
	Injection *injection;
	if(injections.count(name) == 0) {
		injection = new TypedInjection<T>(name);
		injections[name] = injection;
	} else
		injection = injections[name];
	TypedInjection<T> *tInjection = (TypedInjection<T>*)injection;
	tInjection->addListener(fn, i);
	return injection;
}

void uninject(const std::string &name, void *i);

template <typename T> bool injection_point(const std::string &name, T& data) {
	if(injections.count(name) == 1) {
		Injection *injection = injections[name];
		TypedInjection<T> *tInjection = (TypedInjection<T>*)injection;
		return tInjection->apply(data);
	}
	return false;
}

#endif // INJECTION_H