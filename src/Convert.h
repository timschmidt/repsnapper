#pragma once
// File: convert.h
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <stdexcept>

class BadConversion : public std::runtime_error {
public:
	BadConversion(const std::string& s)
		: std::runtime_error(s)
	{ }
};

template<typename T> inline std::string stringify(const T& x)
{
	std::ostringstream o;
	if (!(o << x))
		throw BadConversion(std::string("stringify(") + typeid(x).name() + ")");
	return o.str();
} 
/*

void myCode()
{
Foo x;
...
std::string s = "this is a Foo: " + stringify(x);
...
} */

template<typename T> inline void convert(const std::string& s, T& x, bool failIfLeftoverChars = true)
{
	std::istringstream i(s);
	char c;
	if (!(i >> x) || (failIfLeftoverChars && i.get(c))) throw BadConversion(s);
}

/*
void myCode()
{
	std::string s = ...a string representation of a Foo...;
	...
		Foo x;
	convert(s, x);
	...
		...code that uses x...
} 
*/
template<typename T>
inline T convertTo(const std::string& s, bool failIfLeftoverChars = true)
{
	T x;
	convert(s, x, failIfLeftoverChars);
	return x;
}
/*
#include "convert.h"

void myCode()
{
	std::string a = ...string representation of an int...;
	std::string b = ...string representation of an int...;
	...
		if (convertTo<int>(a) < convertTo<int>(b))
			...;
*/