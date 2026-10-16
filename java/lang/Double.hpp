#include <meta_jni.hpp>

namespace java::lang
{
	KLASS_DECLARATION(Double, "java/lang/Double")
}

#ifndef java_lang_Double
#define java_lang_Double



namespace java::lang
{
	BEGIN_KLASS_MEMBERS(Double)
		jni::method<jdouble, "doubleValue"> doubleValue{ *this };
	END_KLASS_MEMBERS()
}
#endif