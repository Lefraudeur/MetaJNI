#include <iostream>
#include <fstream>
#include "mapping_parser/tiny_mapping_parser.hpp"
#include "mapping_writer/mapping_writer.hpp"

int main(int argc, const char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "usage: " << argv[0] << " <tiny_mapping_file> <obfuscated_namespace> <unobfuscated_namespace>" << std::endl;
		return -1;
	}

	const char* const tiny_mappings_file = argv[1];
	const char* const obfuscated_namespace = argv[2];
	const char* const unobfuscated_namespace = argv[3];

	std::ifstream mapping(tiny_mappings_file, std::ifstream::binary);
	if (!mapping)
	{
		std::cerr << "failed to open tiny_mapping_file" << tiny_mappings_file << std::endl;
		return -1;
	}

	mapping.seekg(0, std::ios::end);
	size_t mapping_size = mapping.tellg();
	mapping.seekg(0, std::ios::beg);

	std::string mappings(mapping_size, '\0');
	mapping.read(mappings.data(), mapping_size);

	tiny_mapping_parser parser(obfuscated_namespace, unobfuscated_namespace, mappings);
	if (!parser) return -1;

	mapping_writer writer(parser);
	// writer.write(); // write each klass in seperate file in different folders based on package name (unfinished)

	std::ofstream file("mappings_auto.hpp");
	writer.write(file);


	return 0;
}