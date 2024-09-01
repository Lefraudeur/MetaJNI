#pragma once
#include <jvmti.h>
#include <string>
#include "../mappings_auto.hpp"

// Requires jni to be attached first
class jvmti
{
public:
	jvmti(JavaVM* jvm);
	~jvmti();

	operator bool();

	// warning: ressource intensive
	java::lang::Class find_loaded_class(const char* class_name);
	std::string get_class_signature(const java::lang::Class& klass);
	java::lang::ClassLoader get_class_ClassLoader(const java::lang::Class& klass);
private:
	jvmtiEnv* jvmti_env = nullptr;
};