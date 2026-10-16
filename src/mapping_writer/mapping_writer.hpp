#pragma once
#include "../mapping_parser/mapping_parser.hpp"
#include <fstream>
#include <unordered_map>

class mapping_writer
{
public:
	mapping_writer(const mapping_parser& parser);

	bool write();
	bool write(std::ostream& output);
private:
	const mapping_parser& parser;
	std::vector<klass> klasses;

	static std::string check_duplicates(const std::string& input, std::unordered_map<std::string, int>& duplicates);
	static std::string reduce_namespace(const std::string& current_namespace, const std::string& input);
	static std::filesystem::path get_relative_path(const std::filesystem::path& root, const std::filesystem::path& path);
	static std::vector<std::string_view> get_path_components(const std::string& path);
	static const klass* get_dependant_klass(klass::type* type);
	static std::filesystem::path get_header_path(const std::string& klass_name);
	static std::string replace_slashes(std::string str);

	struct ID
	{
		int id = 0;
		bool new_gen = false;
	};
	static ID gen_id(const std::string& str);
};