#include "jvmti.hpp"
#include "../meta_jni.hpp"

jvmti::jvmti(JavaVM* jvm)
{
	if (!jvm) return;
	jvm->GetEnv((void**)&jvmti_env, JVMTI_VERSION_1_2);
}

jvmti::~jvmti()
{
	if (!jvmti_env) return;
	jvmti_env->DisposeEnvironment();
}

jvmti::operator bool()
{
	return jvmti_env;
}

maps::Class jvmti::find_loaded_class(const char* class_name)
{
	if (!jvmti_env) return nullptr;

	jint class_count = 0;
	jclass* classes = nullptr;
	jclass found_class = nullptr;
	
	jvmti_env->GetLoadedClasses(&class_count, &classes);
	if (!class_count || !classes) return nullptr;

	for (jint i = 0; i < class_count; ++i)
	{
		std::string signature = get_class_signature(classes[i]);
		if (signature[0] == 'L' && signature.back() == ';')
			signature = signature.substr(1, signature.size() - 2);
		if (signature == class_name)
		{
			found_class = classes[i];
			break;
		}
	}

	// Delete unnecessry local references
	for (jint i = 0; i < class_count; ++i)
		if (classes[i] != found_class)
			jni::get_env()->DeleteLocalRef(classes[i]);

	jvmti_env->Deallocate((unsigned char*)classes);

	return found_class;
}

std::string jvmti::get_class_signature(const maps::Class& klass)
{
	if (!jvmti_env || !klass) return {};

	char* signature_buffer = nullptr;
	jvmti_env->GetClassSignature(klass, &signature_buffer, nullptr);
	if (!signature_buffer) return {};

	std::string signature = signature_buffer;
	jvmti_env->Deallocate((unsigned char*)signature_buffer);

	return signature;
}

maps::ClassLoader jvmti::get_class_ClassLoader(const maps::Class& klass)
{
	jobject classLoader = nullptr;
	jvmti_env->GetClassLoader(klass, &classLoader);
	return maps::ClassLoader(classLoader);
}
