#pragma once
#include "mapping_parser.hpp"
#include <string_view>

class tiny_mapping_parser : public mapping_parser
{
public:
	tiny_mapping_parser(const std::string& obfuscated, const std::string& named, std::string_view mapping);

	operator bool();

	std::vector<klass> get_klasses() const override;
private:

	bool is_error = true;

	std::string obfuscated;
	std::string named;
	std::string_view mapping;

	int get_namespace_index(const std::string& namespace_name) const;
	std::string_view get_string_at_index(std::string_view line, int index) const;
};