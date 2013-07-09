#ifndef VAR_H
#define VAR_H

#include <string>
#include <exception>

namespace utils {

class no_such_var_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "No such variable"; }
};

class illegal_conversion_exception : public std::exception {
public:
	virtual const char *what() const throw() { return "Illegal conversion"; }
};

class Holder {
public:
	virtual const std::type_info& getType() = 0;
	virtual void *getValue() = 0;
};

template <class T> class VHolder : public Holder {
public:
	VHolder(const T &t) : value(t) {}

	virtual const std::type_info &getType() {
		return typeid(value);
	}

	virtual void *getValue() {
		return (void*)&value;
	}

private:
	T value;
};

template <> class VHolder<const char *> : public Holder {
public:
	VHolder(const char *t) : value(t) {
	}

	virtual const std::type_info &getType() {
		return typeid(value);
	}

	virtual void *getValue() {
		return (void*)&value;
	}

private:
	std::string value;
};


class var {
public:

	var() : holder(nullptr) {
	}

	template <typename T> var(T t) {
		holder = new VHolder<T>(t);
	}

	template <typename T> operator T() {
		if(!holder)
			throw no_such_var_exception();
		if(holder->getType() == typeid(T)) {
			const T &t = *((T*)holder->getValue());
			return t;
		}
		throw illegal_conversion_exception();
	}

	operator int() {
		if(!holder)
			throw no_such_var_exception();
		if(holder->getType() == typeid(int)) {
			return *((int*)holder->getValue());
		} else if(holder->getType() == typeid(std::string)) {
			const std::string &s = *((std::string*)holder->getValue());
			char *endptr = nullptr;
			int i = strtol(s.c_str(), &endptr, 0);
			if(endptr == nullptr || *endptr == 0)
				return i;
		}
		throw illegal_conversion_exception();
	}

	operator std::string() {
		if(!holder)
			throw no_such_var_exception();
		if(holder->getType() == typeid(std::string)) {
			return *((std::string*)holder->getValue());
		} else if(holder->getType() == typeid(int)) {
			int i = *((int*)holder->getValue());
			return std::to_string(i);
		}
		throw illegal_conversion_exception();	
	}

private:
	Holder *holder;
};

}

#endif // VAR_H