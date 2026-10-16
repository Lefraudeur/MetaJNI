#include <meta_jni.hpp>

namespace java::util
{
	KLASS_DECLARATION(List, "java/util/List")
}

#ifndef java_util_List
#define java_util_List

#include "Collection.hpp"

namespace java::util
{
	BEGIN_KLASS_MEMBERS_EX(List, Collection)
	END_KLASS_MEMBERS()
}
#endif