#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class klass
{
public:
	klass(const std::string& obfuscated, const std::string& name);
	klass(const klass& other) = delete;
	klass(klass&& other) = default;

	class type
	{
	public:
		enum value_enum
		{
			jvoid,
			jboolean,
			jbyte,
			jchar,
			jshort,
			jint,
			jfloat,
			jlong,
			jdouble,
			jobject,
			jarray
		};

		type(value_enum value);

		static std::unique_ptr<type> signature_to_type(std::string_view signature, const std::vector<klass>& klasses, const std::unordered_map<std::string, std::string>& obf0_to_unobf);

		const value_enum value;
		std::string to_string();
	};

	class klass_type : public type
	{
	public:
		klass_type(const klass& klass);

		const klass& klass;
	};

	class array_type : public type
	{
	public:
		array_type(std::unique_ptr<type> element_type);
		array_type(const array_type& other) = delete;
		array_type(array_type&& other) = default;

		std::unique_ptr<type> element_type;
	};

	class field
	{
	public:
		field(const std::string& obfuscated, const std::string& name, std::unique_ptr<type> type);
		field(const field& other) = delete;
		field(field&& other) = default;

		std::string obfuscated;
		std::string name;
		bool is_static = false;

		std::unique_ptr<type> type;
	};

	class method
	{
	public:
		method(const std::string& obfuscated, const std::string& name, std::unique_ptr<type> return_type, std::vector<std::unique_ptr<type>> parameters);
		method(const method& other) = delete;
		method(method&& other) = default;

		std::string obfuscated;
		std::string name;
		bool is_static = false;

		std::unique_ptr<type> return_type;
		std::vector<std::unique_ptr<type>> parameters;
	};

	class method_signature
	{
	public:
		method_signature(std::string_view method_sig);

		std::vector<std::string_view> parameters;
		std::string_view return_type;

	private:
		std::vector<std::string_view> get_parameters(std::string_view method_sig);
		std::string_view get_return_type(std::string_view method_sig);
	};

	std::vector<field> fields{};
	std::vector<method> methods{};

	std::string obfuscated;
	std::string name;

	std::string namespace_name{};
	std::string namespace_name_end{};

	static std::string filter(std::string unfiltered);
	static std::vector<std::string> decompose_namespace(const std::string& ns, const char* seperator = "::");
private:
	std::pair<std::string, std::string> separe(const std::string& full_class_name);
	std::string filter_namespace(const std::string& unfiltered);
};