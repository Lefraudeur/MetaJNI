#include <meta_jni.hpp>

namespace java::lang
{
	KLASS_DECLARATION(ClassLoader, "java/lang/ClassLoader")
}

#ifndef java_lang_ClassLoader
#define java_lang_ClassLoader

#include "Class.hpp"
#include "String.hpp"

namespace java::lang
{
	BEGIN_KLASS_MEMBERS(ClassLoader)
		jni::method<java::lang::Class, "findClass", jni::NOT_STATIC, String> findClass{ *this };
	END_KLASS_MEMBERS()
}
#endif