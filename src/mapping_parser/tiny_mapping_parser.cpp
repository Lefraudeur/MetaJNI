#include "tiny_mapping_parser.hpp"
#include <unordered_map>
#include <iostream>

tiny_mapping_parser::tiny_mapping_parser(const std::string& obfuscated, const std::string& named, std::string_view mapping) :
	obfuscated(obfuscated),
	named(named),
	mapping(mapping)
{
	std::string_view tiny_header = mapping.substr(0, 9);
	if (tiny_header != "tiny	2	0	") return;

	is_error = false;
}

tiny_mapping_parser::operator bool()
{
	return !is_error;
}

std::vector<klass> tiny_mapping_parser::get_klasses() const
{
	std::vector<klass> klasses{};
	std::unordered_map<std::string, std::string> obf0_to_unobf{}; // for signatures

	int obf = get_namespace_index(obfuscated);
	int unobf = get_namespace_index(named);

	std::vector<std::string_view> klass_maps{};
	for (size_t i = mapping.find("\nc"); i != std::string::npos;)
	{
		size_t next = mapping.find("\nc", i + 1);
		std::string_view klass_map = mapping.substr(i + 1, next - (i + 1));
		klass_maps.push_back(klass_map);
		std::string_view line = mapping.substr(i + 1, mapping.find('\n', i + 1) - (i + 1));
		i = next;

		//+1 to skip 'c'
		std::string_view obf_class_name = get_string_at_index(line, obf + 1);
		std::string_view unobf_class_name = get_string_at_index(line, unobf + 1);
		std::string_view obf0_class_name = get_string_at_index(line, 1);
		if (obf_class_name.empty()) continue;
		if (obf0_class_name.empty()) continue;
		if (unobf_class_name.empty()) unobf_class_name = obf_class_name;

		klasses.push_back(klass{ std::string(obf_class_name), std::string(unobf_class_name) });

		obf0_to_unobf[std::string(obf0_class_name)] = unobf_class_name;
	}

	for (std::string_view klass_map : klass_maps)
	{
		std::vector<std::string_view> signatures{};
		for (size_t prev_i = 0, start = 0, new_i = klass_map.find('\n'); prev_i != std::string::npos; prev_i = new_i, start = prev_i + 1, new_i = klass_map.find('\n', start))
		{
			std::string_view line = klass_map.substr(start, new_i - start);
			if (line.empty()) continue;
			if (line[0] != '\t') continue;
			if (line[1] == 'f')
			{
				std::string_view field = line.substr(1, std::string::npos);
				std::string_view signature = get_string_at_index(field, 1);
				signatures.push_back(signature);
				continue;
			}
			if (line[1] == 'm')
			{
				std::string_view method = line.substr(1, std::string::npos);
				std::string_view signature = get_string_at_index(method, 1);

				klass::method_signature m_sig(signature);
				signatures.insert(signatures.end(), m_sig.parameters.begin(), m_sig.parameters.end());
				signatures.push_back(m_sig.return_type);
				continue;
			}
		}
		for (std::string_view signature : signatures)
		{
			while (signature[0] == '[')
				signature = signature.substr(1, std::string::npos);
			if (signature[0] == 'L' && signature.back() == ';')
			{
				std::string_view obf0_class_name = signature.substr(1, signature.size() - 2);
				if (!obf0_to_unobf.contains(std::string(obf0_class_name)) &&
					std::find_if(klasses.begin(), klasses.end(), [&](klass& k) { return k.obfuscated == obf0_class_name; }) == klasses.end())
					klasses.push_back(klass{ std::string(obf0_class_name) , std::string(obf0_class_name) });
			}
		}
	}

	for (std::string_view klass_map : klass_maps)
	{
		klass* k = nullptr;
		klass::field* current_field = nullptr;
		klass::method* current_method = nullptr;
		for (size_t prev_i = 0, start = 0, new_i = klass_map.find('\n'); prev_i != std::string::npos; prev_i = new_i, start = prev_i + 1, new_i = klass_map.find('\n', start))
		{
			int obf = get_namespace_index(obfuscated);
			int unobf = get_namespace_index(named);
			std::string_view line = klass_map.substr(start, new_i - start);
			if (start == 0)
			{
				std::string_view obf_class_name = get_string_at_index(line, obf + 1);
				std::vector<klass>::iterator it = std::find_if(klasses.begin(), klasses.end(), [&](klass& k) { return k.obfuscated == obf_class_name; });
				if (it != klasses.end())
					k = it._Ptr;
			}
			if (!k) break;
			if (line.empty()) continue;
			if (line[0] != '\t') continue;
			if (line[1] == 'f')
			{
				std::string_view field = line.substr(1, std::string::npos);
				std::string_view signature = get_string_at_index(field, 1);
				std::string_view obf_name = get_string_at_index(field, obf + 2);
				std::string_view unobf_name = get_string_at_index(field, unobf + 2);
				if (obf_name.empty()) continue;
				if (unobf_name.empty()) unobf_name = obf_name;

				std::unique_ptr<klass::type> field_type = klass::type::signature_to_type(signature, klasses, obf0_to_unobf);
				if (!field_type) continue;
				k->fields.push_back(klass::field{ std::string(obf_name), std::string(unobf_name), std::move(field_type) });
				current_field = &k->fields.back();
				current_method = nullptr;
				continue;
			}
			if (line[1] == 'm')
			{
				std::string_view method = line.substr(1, std::string::npos);
				std::string_view signature = get_string_at_index(method, 1);
				std::string_view obf_name = get_string_at_index(method, obf + 2);
				std::string_view unobf_name = get_string_at_index(method, unobf + 2);
				if (obf_name.empty()) continue;
				if (unobf_name.empty()) unobf_name = obf_name;


				klass::method_signature m_sig(signature);
				std::vector<std::unique_ptr<klass::type>> parameters{};
				parameters.reserve(m_sig.parameters.size());
				for (std::string_view parameter : m_sig.parameters)
					parameters.push_back(klass::type::signature_to_type(parameter, klasses, obf0_to_unobf));
				std::unique_ptr<klass::type> return_type = klass::type::signature_to_type(m_sig.return_type, klasses, obf0_to_unobf);

				k->methods.push_back(klass::method(std::string(obf_name), std::string(unobf_name), std::move(return_type), std::move(parameters)));
				current_method = &k->methods.back();
				current_field = nullptr;
				continue;
			}
			if (line == "\t\tc\tstatic")
			{
				if (current_method)
					current_method->is_static = true;
				if (current_field)
					current_field->is_static = true;
				continue;
			}
		}
	}



	return klasses;
}

int tiny_mapping_parser::get_namespace_index(const std::string& namespace_name) const
{
	std::string_view line = mapping.substr(9, mapping.find('\n') - 9);

	for (size_t prev_i = 0, start = 0, new_i = line.find('\t'), index = 0; prev_i != std::string::npos; prev_i = new_i, start = prev_i + 1, new_i = line.find('\t', start), ++index)
	{
		std::string_view current_namespace = line.substr(start, new_i - start);
		if (current_namespace == namespace_name) return index;
	}
	return -1;
}

std::string_view tiny_mapping_parser::get_string_at_index(std::string_view line, int index) const
{
	for (size_t prev_i = 0, start = 0, new_i = line.find('\t'), i = 0; prev_i != std::string::npos; prev_i = new_i, start = prev_i + 1, new_i = line.find('\t', start), ++i)
	{
		std::string_view string = line.substr(start, new_i - start);
		if (i == index)
			return string;
	}
	return std::string_view();
}
