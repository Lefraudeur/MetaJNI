#include <meta_jni.hpp>

namespace java::net
{
	KLASS_DECLARATION(URL, "java/net/URL")
}

#ifndef java_net_URL
#define java_net_URL

#include "../lang/String.hpp"

namespace java::net
{
	BEGIN_KLASS_MEMBERS(URL)
		jni::constructor<java::lang::String> constructor{ *this };
		jni::method<java::lang::String, "toString"> toString{ *this };
	END_KLASS_MEMBERS()
}
#endif