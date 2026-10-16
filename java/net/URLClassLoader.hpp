#include <meta_jni.hpp>

namespace java::net
{
	KLASS_DECLARATION(URLClassLoader, "java/net/URLClassLoader")
}

#ifndef java_net_URLClassLoader
#define java_net_URLClassLoader

#include "../lang/ClassLoader.hpp"
#include "URL.hpp"

namespace java::net
{
	BEGIN_KLASS_MEMBERS_EX(URLClassLoader, java::lang::ClassLoader)
		jni::constructor<jni::array<java::net::URL>> constructor{ *this };
		jni::constructor<jni::array<java::net::URL>, java::lang::ClassLoader> constructor2{ *this };

		jni::method<void, "addURL", jni::NOT_STATIC, java::net::URL> addURL{ *this };
		jni::method<jni::array<java::net::URL>, "getURLs", jni::NOT_STATIC> getURLs{ *this };
	END_KLASS_MEMBERS()
}
#endif