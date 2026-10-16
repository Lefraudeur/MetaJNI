#include "klass.hpp"
#include <iostream>
#include <algorithm>

klass::klass(const std::string& obfuscated, const std::string& name) :
	obfuscated(obfuscated),
	name(name)
{
	std::pair<std::string, std::string> separed = separe(name);
	namespace_name = filter_namespace(separed.first);
	namespace_name_end = filter(separed.second);
}

std::pair<std::string, std::string> klass::separe(const std::string& full_class_name)
{
	std::string package = "defpackage";
	size_t last_slash = full_class_name.rfind('/');
	std::string name = full_class_name;
	if (last_slash != std::string::npos)
	{
		name = full_class_name.substr(last_slash + 1, std::string::npos);
		package = full_class_name.substr(0, last_slash);
		std::string old_package = std::move(package);
		package = {};
		for (const char& c : old_package)
		{
			if (c == '/')
			{
				package += "::";
				continue;
			}
			package += c;
		}
	}
	return std::pair<std::string, std::string>(package, name);
}

std::vector<std::string> klass::decompose_namespace(const std::string& ns, const char* seperator)
{
	std::vector<std::string> decomposed{};

	for (size_t i = 0; i != std::string::npos; )
	{
		size_t next = ns.find(seperator, i);
		decomposed.push_back(ns.substr(i, next - i));
		i = next;
		if (i != std::string::npos)
			i += std::strlen(seperator);
	}

	return decomposed;
}

std::string klass::filter_namespace(const std::string& unfiltered)
{
	std::string filtered{};
	std::vector<std::string> decomposed = decompose_namespace(unfiltered);
	for (int i = 0; i < decomposed.size(); ++i)
	{
		filtered += filter(decomposed[i]);
		if (i != decomposed.size() - 1)
			filtered += "::";
	}
	return filtered;
}

klass::type::type(value_enum value) : 
	value(value)
{
}

std::unique_ptr<klass::type> klass::type::signature_to_type(std::string_view signature, const std::vector<klass>& klasses, const std::unordered_map<std::string, std::string>& obf0_to_unobf)
{
	switch (signature[0])
	{
	case 'V':
		return std::make_unique<type>(jvoid);
	case 'Z':
		return std::make_unique<type>(jboolean);
	case 'B':
		return std::make_unique<type>(jbyte);
	case 'C':
		return std::make_unique<type>(jchar);
	case 'S':
		return std::make_unique<type>(jshort);
	case 'I':
		return std::make_unique<type>(jint);
	case 'F':
		return std::make_unique<type>(jfloat);
	case 'J':
		return std::make_unique<type>(jlong);
	case 'D':
		return std::make_unique<type>(jdouble);
	case 'L':
	{
		std::string_view class_name = signature.substr(1, signature.size() - 2);
		if (obf0_to_unobf.contains(std::string(class_name)))
			class_name = obf0_to_unobf.at(std::string(class_name));
		const klass& k = *std::find_if(klasses.begin(), klasses.end(), [&](const klass& k) { return k.name == class_name; });
		return std::make_unique<klass_type>(k);
	}
	case '[':
	{
		std::string_view class_name = signature.substr(1, std::string::npos);
		return std::make_unique<array_type>(signature_to_type(class_name, klasses, obf0_to_unobf));
	}
	}
	return nullptr;
}

std::string klass::type::to_string()
{
	switch (value)
	{
	case jvoid:
		return "void";
	case jboolean:
		return "jboolean";
	case jbyte:
		return "jbyte";
	case jchar:
		return "jchar";
	case jshort:
		return "jshort";
	case jint:
		return "jint";
	case jfloat:
		return "jfloat";
	case jlong:
		return "jlong";
	case jdouble:
		return "jdouble";
	case jobject:
		return ((klass_type*)this)->klass.namespace_name + "::" +  ((klass_type*)this)->klass.namespace_name_end;
	case jarray:
		return "jni::array<" + ((array_type*)this)->element_type->to_string() + ">";
	}
	return std::string();
}

klass::klass_type::klass_type(const ::klass& klass) :
	type(jobject),
	klass(klass)
{
}

klass::field::field(const std::string& obfuscated, const std::string& name, std::unique_ptr<klass::type> type) : 
	obfuscated(obfuscated),
	name(name),
	type(std::move(type))
{
}

klass::method::method(const std::string& obfuscated, const std::string& name, std::unique_ptr<type> return_type, std::vector<std::unique_ptr<type>> parameters) :
	obfuscated(obfuscated),
	name(name),
	return_type(std::move(return_type)),
	parameters(std::move(parameters))
{
}

klass::array_type::array_type(std::unique_ptr<type> element_type) : 
	type(jarray),
	element_type(std::move(element_type))
{
}

klass::method_signature::method_signature(std::string_view method_sig) :
	parameters(get_parameters(method_sig)),
	return_type(get_return_type(method_sig))
{
}

std::vector<std::string_view> klass::method_signature::get_parameters(std::string_view method_sig)
{
	size_t start = method_sig.find('(') + 1;
	std::string_view parameters_str = method_sig.substr(start, method_sig.find(')') - start);
	std::vector<std::string_view> params{};

	for (int i = 0; i < parameters_str.size();)
	{
		if (parameters_str[i] == 'L')
		{
			size_t end = parameters_str.find(';', i);
			params.push_back(parameters_str.substr(i, end + 1 - i));
			i = end + 1;
		}
		else if (parameters_str[i] == '[')
		{
			size_t end = parameters_str.find_first_not_of('[', i);
			if (parameters_str[end] == 'L')
				end = parameters_str.find(';', end);
			params.push_back(parameters_str.substr(i, end + 1 - i));
			i = end + 1;
		}
		else
		{
			params.push_back(parameters_str.substr(i, 1));
			i++;
		}
	}

	return params;
}

std::string_view klass::method_signature::get_return_type(std::string_view method_sig)
{
	return method_sig.substr(method_sig.rfind(')') + 1, std::string::npos);
}

std::string klass::filter(std::string unfiltered)
{
	const char* illegal[]
	{
		"mutable",
		"template",
		"extern",
		"register",
		"ERROR_INVALID_OPERATION",
		"ERROR_EMPTY",
		"ERROR",
		"and",
		"or",
		"REGISTERED",
		"requires",
		"THIS",
		"THIS_",
		"FALSE",
		"TRUE",
		"INFINITY",
		"small",
		"export",
		"NULL",
		"ERROR_UNKNOWN_PROPERTY",
		"namespace",
		"ERROR_ALREADY_EXISTS",
		"ERROR_INVALID_INDEX",
		"VOID",
		"delete",
		"ERROR_REMOVE_FAILED",
		"auto",
		"IGNORE",
		"operator",
		"friend",
		"union",
		"not",
		"CREATE_NEW",
		"RELATIVE",
		"xor",
		"INFINITE",
		"PASSTHROUGH",
		"ABSOLUTE",
		"unsigned",
		"bool",
		"DEFAULT_QUALITY",
		"MOD_CONTROL"
	};
	if (std::any_of(std::begin(illegal), std::end(illegal), [&](const char* element) {return unfiltered == element; }))
		unfiltered += '_$';

	std::string_view blacklisted_chars = "-<>";
	for (size_t i = unfiltered.find_first_of(blacklisted_chars); i != std::string::npos; i = unfiltered.find_first_of(blacklisted_chars, i + blacklisted_chars.size()))
		unfiltered[i] = '_$';

	return unfiltered;
}