#include <meta_jni.hpp>

namespace java::util
{
	KLASS_DECLARATION(Collection, "java/util/Collection")
}

#ifndef java_util_Collection
#define java_util_Collection

#include "../lang/Object.hpp"

namespace java::util
{
	BEGIN_KLASS_MEMBERS(Collection)
		jni::method<jni::array<java::lang::Object>, "toArray"> toArray{ *this };
	END_KLASS_MEMBERS()
}
#endif