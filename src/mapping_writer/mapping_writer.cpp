#include "mapping_writer.hpp"
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <filesystem>

mapping_writer::mapping_writer(const mapping_parser& parser) : 
	parser(parser),
	klasses(parser.get_klasses())
{
}

std::filesystem::path mapping_writer::get_header_path(const std::string& klass_name)
{
	std::filesystem::path path{ klass_name };

	if (!path.has_parent_path())
		path = "defpackage/" + path.string();

	path += ".hpp";

	return "output/" + path.string();
}

std::string mapping_writer::replace_slashes(std::string str)
{
	for (char& c : str)
		if (c == '/')
			c = '_';
	return str;
}

mapping_writer::ID mapping_writer::gen_id(const std::string& str)
{
	static std::unordered_map<std::string, int> ids{};
	static int next_id = 1;
	int& id = ids[str];
	if (id)
		return { id, true };
	id = next_id;
	++next_id;
	return { id, false };
}

const klass* mapping_writer::get_dependant_klass(klass::type* type)
{
	if (type->value == klass::type::jobject)
		return &(((klass::klass_type*)type)->klass);
	if (type->value == klass::type::jarray)
		return get_dependant_klass(((klass::array_type*)type)->element_type.get());
	return nullptr;
}

std::vector<std::string_view> mapping_writer::get_path_components(const std::string& path)
{
	std::vector<std::string_view> components{};
	for (size_t i = 0; i != std::string::npos;)
	{
		size_t next = path.find('/', i);
		std::string_view component = std::string_view(path).substr(i, next - i);
		components.push_back(component);
		i = next;
		if (i != std::string::npos)
			i += 1;
	}
	return components;
}

std::filesystem::path mapping_writer::get_relative_path(const std::filesystem::path& root, const std::filesystem::path& path)
{
	std::string r = root.string();
	std::string p = path.string();

	std::vector<std::string_view> r_components = get_path_components(r);
	std::vector<std::string_view> p_components = get_path_components(p);

	std::string new_path{};
	bool consequetive = true;
	size_t ident = 0;
	for (size_t i = 0; i < p_components.size(); ++i)
	{
		if (consequetive && i < r_components.size() && p_components[i] == r_components[i])
		{
			ident++;
			continue;
		}
		consequetive = false;
		new_path += p_components[i];
		if (i < p_components.size() - 1)
			new_path += "/";
	}
	
	size_t nest = r_components.size() - ident;
	for (size_t i = 0; i < nest; ++i)
		new_path = "../" + new_path;

	return new_path;
}

std::string mapping_writer::reduce_namespace(const std::string& current_namespace, const std::string& input)
{
	size_t new_start = 0;
	for (size_t i = 0; i != std::string::npos;)
	{
		size_t next = input.find("::", i);

		std::string_view component1 = std::string_view(input).substr(i, next - i);
		if (i < current_namespace.size())
		{
			std::string_view component2 = std::string_view(current_namespace).substr(i, next - i);
			if (component1 == component2)
			{
				i = next;
				if (i != std::string::npos)
					i += 2;
				new_start = i;
				continue;
			}
		}
		break;
	}

	if (new_start == std::string::npos)
		return std::string();

	return input.substr(new_start);
}

bool mapping_writer::write()
{
	for (const klass& k : klasses)
	{
		std::unordered_map<std::string, int> duplicates{};

		std::filesystem::path path = get_header_path(k.name);

		std::vector<const klass*> dependant_klasses{};
		std::vector<klass::type*> types_to_check{};
		for (const klass::field& f : k.fields)
			types_to_check.push_back(f.type.get());
		for (const klass::method& m : k.methods)
		{
			types_to_check.push_back(m.return_type.get());
			for (const std::unique_ptr<klass::type>& type : m.parameters)
				types_to_check.push_back(type.get());
		}
		for (klass::type* t : types_to_check)
			if (const klass* dependant = get_dependant_klass(t))
				if (std::find(dependant_klasses.begin(), dependant_klasses.end(), dependant) == dependant_klasses.end())
					dependant_klasses.push_back(dependant);

		std::filesystem::create_directories(path.parent_path());
		std::ofstream header{ path };

		std::stringstream buff{};

		buff << R"(#include <meta_jni.hpp>)""\n\n";

		buff << "namespace " << k.namespace_name << '\n'
			<< "{\n";
		buff << "	KLASS_DECLARATION(" << k.namespace_name_end << ", \"" << k.obfuscated << "\")\n";
		buff << "}\n\n";

		std::string define = k.name;
		for (char& c : define)
			if (c == '/' || c == '-')
				c = '_';
		buff << "#ifndef " << define << '\n';
		buff << "#define " << define << "\n\n";

		for (const klass* dependant : dependant_klasses)
		{
			if (dependant != &k)
				buff << "#include \"" + get_relative_path(path.parent_path(), get_header_path(dependant->name)).string() + "\"\n";
		}

		buff << "\n";

		
		for (const klass::field& f : k.fields)
		{
			std::string field_args = f.type->to_string() + ", \"" + f.obfuscated + "\", " + (f.is_static ? "jni::STATIC" : "jni::NOT_STATIC");
			buff << "extern template class jni::field<" + field_args + ">;\n";
		}
		for (const klass::method& m : k.methods)
		{
			std::string parameters_str{};
			for (const std::unique_ptr<klass::type>& p : m.parameters)
				parameters_str += ", " + p->to_string();
			std::string method_args = m.return_type->to_string() + ", \"" + m.obfuscated + "\", " + (m.is_static ? "jni::STATIC" : "jni::NOT_STATIC");
			buff << "extern template class jni::method<" + method_args + ">;\n";
		}
		

		buff << "\n";

		buff << "namespace " << k.namespace_name << '\n'
			<< "{\n";

		buff << "	BEGIN_KLASS_MEMBERS(" << k.namespace_name_end << ")\n";
		for (const klass::field& f : k.fields)
		{
			buff << "		jni::field<" << f.type->to_string() << ", \"" << f.obfuscated << "\", " << (f.is_static ? "jni::STATIC" : "jni::NOT_STATIC") << "> " << check_duplicates(klass::filter(f.name), duplicates) << "{ *this };\n";
		}
		for (const klass::method& m : k.methods)
		{
			std::string parameters_str{};
			for (const std::unique_ptr<klass::type>& p : m.parameters)
				parameters_str += ", " + p->to_string();
			buff << "		jni::method<" << m.return_type->to_string() << ", \"" << m.obfuscated << "\", " << (m.is_static ? "jni::STATIC" : "jni::NOT_STATIC") << parameters_str << "> " << check_duplicates(klass::filter(m.name), duplicates) << "{ *this };\n";
		}
		buff << "	END_KLASS_MEMBERS()\n";

		buff << "}\n";
		buff << "#endif\n";

		header << buff.view();

		std::stringstream buff2{};

		buff2 << "#include \"" << path.filename().string() << "\"\n"
			"#ifdef SRC\n";

		for (const klass* dependant : dependant_klasses)
		{
			if (dependant != &k)
				buff2 << "#include \"" + get_relative_path(path.parent_path(), get_header_path(dependant->name)).replace_extension(".mhpp").string() + "\"\n";
		}

		std::vector<int> ids{};
		for (const klass::field& f : k.fields)
		{
			std::string field_args = f.type->to_string() + ", \"" + f.obfuscated + "\", " + (f.is_static ? "jni::STATIC" : "jni::NOT_STATIC");
			ID id = gen_id(field_args);
			if (std::find(ids.begin(), ids.end(), id.id) == ids.end())
			{
				buff2 << "#ifndef id_" << id.id << '\n'
					<< "#define id_" << id.id << '\n'
					<< "template class jni::field<" + field_args + ">;\n"
					<< "#endif\n";
				ids.push_back(id.id);
			}
		}
		ids.clear();
		for (const klass::method& m : k.methods)
		{
			std::string parameters_str{};
			for (const std::unique_ptr<klass::type>& p : m.parameters)
				parameters_str += ", " + p->to_string();
			std::string method_args = m.return_type->to_string() + ", \"" + m.obfuscated + "\", " + (m.is_static ? "jni::STATIC" : "jni::NOT_STATIC");
			ID id = gen_id(method_args);
			if (std::find(ids.begin(), ids.end(), id.id) == ids.end())
			{
				buff2 << "#ifndef id_" << id.id << '\n'
					<< "#define id_" << id.id << '\n'
					<< "template class jni::method<" + method_args + ">;\n"
					<< "#endif\n";
				ids.push_back(id.id);
			}
		}

		buff2 << "#endif";

		std::ofstream src{ path.replace_extension(".mhpp") };

		src << buff2.view();
	}

	return false;
}

bool mapping_writer::write(std::ostream& output)
{
	std::stringstream code{};

	code << "#pragma once\n"
		R"(#include <meta_jni.hpp>)""\n\n";

	{
		std::unordered_map<std::string, std::stringstream> namespaces{};

		for (const klass& k : klasses)
		{
			std::string package = k.namespace_name;
			std::string name = k.namespace_name_end;

			std::stringstream& ns = namespaces[package];

			if (ns.tellp() == 0)
				ns << "namespace " << package << '\n'
				<< "{\n";

			ns << "	KLASS_DECLARATION(" << name << R"(, ")" << k.obfuscated << R"(");)""\n";
		}

		for (std::pair<const std::string, std::stringstream>& p : namespaces)
		{
			p.second << "}\n";
			code << p.second.str();
		}
	}

	{
		std::unordered_map<std::string, std::stringstream> namespaces{};

		for (const klass& k : klasses)
		{
			std::unordered_map<std::string, int> duplicates{};
			std::string package = k.namespace_name;
			std::string name = k.namespace_name_end;

			std::stringstream& ns = namespaces[package];

			if (ns.tellp() == 0)
				ns << "namespace " << package << '\n'
				<< "{\n";

			ns << "	BEGIN_KLASS_MEMBERS(" << name << ")\n";
			for (const klass::field& f : k.fields)
			{
				ns << "		jni::field<" << f.type->to_string() << ", \"" << f.obfuscated << "\", " << (f.is_static ? "jni::STATIC" : "jni::NOT_STATIC") << "> " << check_duplicates(klass::filter(f.name), duplicates) << "{ *this };\n";
			}
			for (const klass::method& m : k.methods)
			{
				std::string parameters_str{};
				for (const std::unique_ptr<klass::type>& p : m.parameters)
					parameters_str += ", " + p->to_string();
				ns << "		jni::method<" << m.return_type->to_string() << ", \"" << m.obfuscated << "\", " << (m.is_static ? "jni::STATIC" : "jni::NOT_STATIC") << parameters_str << "> " << check_duplicates(klass::filter(m.name), duplicates) << "{ *this };\n";
			}
			ns << "	END_KLASS_MEMBERS()\n";
		}

		for (std::pair<const std::string, std::stringstream>& p : namespaces)
		{
			p.second << "}\n";
			code << p.second.str();
		}
	}


	output << code.str();

	return false;
}

std::string mapping_writer::check_duplicates(const std::string& input, std::unordered_map<std::string, int>& duplicates)
{
	std::string output = input;
	int& count = duplicates[input];

	if (count)
		output += "_" + std::to_string(count);

	count++;
	return output;
}
