#include <meta_jni.hpp>

namespace java::lang
{
	KLASS_DECLARATION(String, "java/lang/String")
}

#ifndef java_lang_String
#define java_lang_String



namespace java::lang
{
	BEGIN_KLASS_MEMBERS(String)
		static String create(const char* str)
		{
			return String(jni::get_env()->NewStringUTF(str));
		}


		std::string to_string() const
		{
			if (!object_instance) return std::string();
			jstring str_obj = (jstring)object_instance;
			jsize utf8_size = jni::get_env()->GetStringUTFLength(str_obj);
			jsize size = jni::get_env()->GetStringLength(str_obj);

			std::string str(utf8_size, '\0');
			jni::get_env()->GetStringUTFRegion(str_obj, 0, size, str.data());
			return str;
		}
	END_KLASS_MEMBERS()
}
#endif