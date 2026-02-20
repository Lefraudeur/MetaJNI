#pragma once

#include "meta_jni.hpp"
#include<string>

namespace maps
{
	BEGIN_KLASS_DEF(Object, "java/lang/Object")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(String, "java/lang/String")
		static String create(const char* str)
		{
			return String(jni::get_env()->NewStringUTF(str));
		}


		std::string to_string()
		{
			if (!get_jobject()) return std::string();
			jstring str_obj = (jstring)get_jobject();
			jsize utf8_size = jni::get_env()->GetStringUTFLength(str_obj);
			jsize size = jni::get_env()->GetStringLength(str_obj);

			std::string str(utf8_size, '\0');
			jni::get_env()->GetStringUTFRegion(str_obj, 0, size, str.data());
			return str;
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Collection, "java/util/Collection")
		method<jni::array<Object>, "toArray"> toArray{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(List, "java/util/List", Collection)
	END_KLASS_DEF()
	BEGIN_KLASS_DEF(URL, "java/net/URL")
		constructor<String> init{ *this };

		method<String, "toString"> toString{ *this };
	END_KLASS_DEF()


	BEGIN_KLASS_DEF(Entity, "pk")
		method<String, "e_"> getName{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(EntityLivingBase, "pr", Entity)
		method<jfloat, "bn"> getHealth{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(EntityPlayer, "wn", EntityLivingBase)
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(EntityPlayerSP, "bew", EntityPlayer)
		method<void, "e", String> sendChatMessage{ *this };
		method<String, "w"> getClientBrand{ *this };
	END_KLASS_DEF()


	BEGIN_KLASS_DEF(World, "adm")
		field<List, "j"> playerEntities{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(WorldClient, "bdb", World)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Minecraft, "ave")
		inline static static_field<Minecraft, "S"> theMinecraft{};
		field<jint, "d"> displayWidth{ *this };
		field<EntityPlayerSP, "h"> thePlayer{ *this };
		field<WorldClient, "f"> theWorld{ *this };

		method<void, "aw"> clickMouse{ *this };
		method<void, "a", jint, jint> resize{ *this };
	END_KLASS_DEF()

	KLASS_DECLARATION(ClassLoader, "java/lang/ClassLoader");

	BEGIN_KLASS_MEMBERS(ClassLoader)
	END_KLASS_MEMBERS()
}