#pragma once
#include "klass/klass.hpp"

class mapping_parser
{
public:
	mapping_parser();
	virtual std::vector<klass> get_klasses() const = 0;
};