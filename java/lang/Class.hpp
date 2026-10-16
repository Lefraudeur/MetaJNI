#include <meta_jni.hpp>

namespace java::lang
{
	KLASS_DECLARATION(Class, "java/lang/Class")
}

#ifndef java_lang_Class
#define java_lang_Class



namespace java::lang
{
	BEGIN_KLASS_MEMBERS(Class)
		operator jclass() const
		{
			return (jclass)object_instance;
		}
	END_KLASS_MEMBERS()
}
#endif