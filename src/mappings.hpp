#pragma once

#include "meta_jni.hpp"
#include<string>
#include "maths/maths.hpp"

namespace maps
{
	BEGIN_KLASS_DEF(Object, "java/lang/Object")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Class, "java/lang/Class")
		operator jclass() const
		{
			return (jclass)object_instance;
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(String, "java/lang/String")
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
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Collection, "java/util/Collection")
		jni::method<jni::array<Object>, "toArray"> toArray{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(List, "java/util/List", Collection)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(URL, "java/net/URL")
		jni::constructor<String> constructor{ *this };

		jni::method<String, "toString"> toString{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(ClassLoader, "java/lang/ClassLoader");
		jni::method<Class, "findClass", jni::NOT_STATIC, String> findClass{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(URLClassLoader, "java/net/URLClassLoader", ClassLoader)
		jni::constructor<jni::array<URL>> constructor{ *this };
		jni::constructor<jni::array<URL>, ClassLoader> constructor2{ *this };

		jni::method<void, "addURL", jni::NOT_STATIC, URL> addURL{ *this };
		jni::method<jni::array<URL>, "getURLs", jni::NOT_STATIC> getURLs{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Double, "java/lang/Double")
		jni::method<jdouble, "doubleValue"> doubleValue{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(PrintStream, "java/io/PrintStream")
		jni::method<void, "println", jni::NOT_STATIC, String> println{ *this };
	END_KLASS_DEF()

		BEGIN_KLASS_DEF(System, "java/lang/System")
		jni::field<PrintStream, "out", jni::STATIC> out{ *this };
	END_KLASS_DEF()




	BEGIN_KLASS_DEF(Vec3d, "net/minecraft/world/phys/Vec3")
		jni::field<jdouble, "x"> x{ *this };
		jni::field<jdouble, "y"> y{ *this };
		jni::field<jdouble, "z"> z{ *this };

		maths::vector3d to_vector3d()
		{
			return { x.get(), y.get(), z.get() };
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Item, "net/minecraft/world/item/Item")
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(SwordItem, "net/minecraft/world/item/SwordItem", Item)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(ItemStack, "net/minecraft/world/item/ItemStack")
		jni::field<Item, "item"> item{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Box, "net/minecraft/world/phys/AABB")
		jni::field<jdouble, "minX"> minX{ *this };
		jni::field<jdouble, "minY"> minY{ *this };
		jni::field<jdouble, "minZ"> minZ{ *this };
		jni::field<jdouble, "maxX"> maxX{ *this };
		jni::field<jdouble, "maxY"> maxY{ *this };
		jni::field<jdouble, "maxZ"> maxZ{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(EntityPose, "net/minecraft/world/entity/Pose")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(EntityDimensions, "net/minecraft/world/entity/EntityDimensions")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffect, "net/minecraft/world/effect/MobEffect")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffects, "net/minecraft/world/effect/MobEffects")
		jni::field<StatusEffect, "GLOWING", jni::STATIC> GLOWING{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffectInstance, "net/minecraft/world/effect/MobEffectInstance")
		jni::constructor<StatusEffect, jint, jint> constructor{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Entity, "net/minecraft/world/entity/Entity")
		jni::field<Box, "bb"> boundingBox{ *this };
		jni::field<jint, "tickCount"> age{ *this };
		jni::field<Vec3d, "position"> pos{ *this };
		jni::field<jfloat, "yRot"> yaw{ *this };
		jni::field<jfloat, "xRot"> pitch{ *this };
		jni::field<jfloat, "yRotO"> prevYaw{ *this };
		jni::field<jfloat, "xRotO"> prevPitch{ *this };
		jni::field<jfloat, "fallDistance"> fallDistance{ *this };
		jni::field<jboolean, "onGround"> onGround{ *this };

		jni::field<jdouble, "xOld"> lastRenderX{ *this };
		jni::field<jdouble, "yOld"> lastRenderY{ *this };
		jni::field<jdouble, "zOld"> lastRenderZ{ *this };

		maths::vector3d get_prev_position()
		{
			return { lastRenderX.get(), lastRenderY.get(), lastRenderZ.get() };
		}

		maths::vector3d get_position()
		{
			return pos.get().to_vector3d();
		}

		maths::angles get_prev_angles()
		{
			return { prevYaw.get(), prevPitch.get() };
		}

		maths::angles get_angles()
		{
			return { yaw.get(), pitch.get() };
		}

		jni::method<EntityPose, "getPose"> getPose{ *this };
		jni::method<EntityDimensions, "getDimensions", jni::NOT_STATIC, EntityPose> getDimensions{ *this };
		jni::method<jfloat, "getEyeHeight", jni::NOT_STATIC, EntityPose> getEyeHeight{ *this };
		jni::method<jboolean, "isAlive"> isAlive{ *this };
		jni::method<void, "setGlowingTag", jni::NOT_STATIC, jboolean> setGlowing{ *this };
		jni::method<void, "setSharedFlag", jni::NOT_STATIC, jint, jboolean> setFlag{ *this };
		jni::method<void, "setSprinting", jni::NOT_STATIC, jboolean> setSprinting{ *this };
		jni::method<jboolean, "isSprinting"> isSprinting{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(LivingEntity, "net/minecraft/world/entity/LivingEntity", Entity)
		jni::field<jint, "attackStrengthTicker"> lastAttackedTicks{ *this };

		jni::method<jboolean, "isHolding", jni::NOT_STATIC, Item> isHolding{ *this };
		jni::method<ItemStack, "getMainHandItem"> getMainHandStack{ *this };
		jni::method<jboolean, "addEffect", jni::NOT_STATIC, StatusEffectInstance> addStatusEffect{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(PlayerEntity, "net/minecraft/world/entity/player/Player", LivingEntity)
		jni::method<jfloat, "getAttackStrengthScale", jni::NOT_STATIC, jfloat> getAttackCooldownProgress{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(AbstractClientPlayerEntity, "net/minecraft/client/player/AbstractClientPlayer", PlayerEntity)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(ClientPlayerEntity, "net/minecraft/client/player/LocalPlayer", AbstractClientPlayerEntity)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(ClientWorld, "net/minecraft/client/multiplayer/ClientLevel")
		jni::field<List, "players"> players{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(SimpleOption, "net/minecraft/client/OptionInstance")
		jni::field<Object, "value"> value{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(GameOptions, "net/minecraft/client/Options")
		jni::field<SimpleOption, "sensitivity"> mouseSensitivity{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Mouse, "net/minecraft/client/MouseHandler")
		jni::field<jdouble, "accumulatedDX"> cursorDeltaX{ *this };
		jni::field<jdouble, "accumulatedDY"> cursorDeltaY{ *this };

		jni::method<void, "turnPlayer"> updateMouse{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(RenderTickCounter, "net/minecraft/client/Timer")
		jni::field<jfloat, "partialTick"> tickDelta{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(HitResult$Type, "net/minecraft/world/phys/HitResult$Type")
		jni::field<HitResult$Type, "ENTITY", jni::STATIC> ENTITY{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(HitResult, "net/minecraft/world/phys/HitResult")
		jni::method<HitResult$Type, "getType"> getType{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(EntityHitResult, "net/minecraft/world/phys/EntityHitResult", HitResult)
		jni::field<Entity, "entity"> entity{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(MinecraftClient, "net/minecraft/client/Minecraft")
		jni::field<MinecraftClient, "instance", jni::STATIC> instance{ *this };

		jni::field<ClientWorld, "level"> world{ *this };
		jni::field<ClientPlayerEntity, "player"> player{ *this };
		jni::field<HitResult, "hitResult"> crosshairTarget{ *this };

		jni::field<Mouse, "mouseHandler"> mouse{ *this };
		jni::field<GameOptions, "options"> options{ *this };
		jni::field<RenderTickCounter, "timer"> renderTickCounter{ *this };
	END_KLASS_DEF()
}