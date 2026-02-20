#pragma once

#include "meta_jni.hpp"
#include<string>
#include "maths/maths.hpp"
#include <glm/mat4x4.hpp>

namespace maps
{
	KLASS_DECLARATION(String, "java/lang/String");
	// jdk mappings, should never change
	BEGIN_KLASS_DEF(Object, "java/lang/Object")
		method<String, "toString"> toString{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Class, "java/lang/Class")
		operator jclass() const
		{
			return (jclass)get_jobject();
		}
	END_KLASS_DEF()

	BEGIN_KLASS_MEMBERS(String)
		inline static String create(const char* str)
		{
			return String(jni::get_env()->NewStringUTF(str));
		}


		inline std::string to_string() const
		{
			if (!get_jobject()) return std::string();
			jstring str_obj = (jstring)get_jobject();
			jsize utf8_size = jni::get_env()->GetStringUTFLength(str_obj);
			jsize size = jni::get_env()->GetStringLength(str_obj);

			std::string str(utf8_size, '\0');
			jni::get_env()->GetStringUTFRegion(str_obj, 0, size, str.data());
			return str;
		}
	END_KLASS_MEMBERS()

	BEGIN_KLASS_DEF(Collection, "java/util/Collection")
		method<jni::array<Object>, "toArray"> toArray{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(List, "java/util/List", Collection)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(URL, "java/net/URL")
		constructor<String> init{ *this };

		method<String, "toString"> toString{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(ClassLoader, "java/lang/ClassLoader");
		method<Class, "findClass", String> findClass{ *this };
		method<Class, "loadClass", String> loadClass{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(URLClassLoader, "java/net/URLClassLoader", ClassLoader)
		constructor<jni::array<URL>> init{ *this };
		constructor<jni::array<URL>, ClassLoader> init2{ *this };

		method<void, "addURL", URL> addURL{ *this };
		method<jni::array<URL>, "getURLs"> getURLs{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Double, "java/lang/Double")
		method<jdouble, "doubleValue"> doubleValue{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF(Boolean, "java/lang/Boolean")
		method<jboolean, "booleanValue"> booleanValue{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF(Integer, "java/lang/Integer")
		method<jint, "intValue"> intValue{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(PrintStream, "java/io/PrintStream")
		method<void, "println", String> println{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(System, "java/lang/System")
		inline static static_field<PrintStream, "out"> out{};
	END_KLASS_DEF()


	// minecraft external libraries, also shouldn't change

	BEGIN_KLASS_DEF(Matrix4f, "org/joml/Matrix4f")
		field<jfloat, "m00"> m00{*this};
		field<jfloat, "m01"> m01{ *this };
		field<jfloat, "m02"> m02{ *this };
		field<jfloat, "m03"> m03{ *this };
		field<jfloat, "m10"> m10{ *this };
		field<jfloat, "m11"> m11{ *this };
		field<jfloat, "m12"> m12{ *this };
		field<jfloat, "m13"> m13{ *this };
		field<jfloat, "m20"> m20{ *this };
		field<jfloat, "m21"> m21{ *this };
		field<jfloat, "m22"> m22{ *this };
		field<jfloat, "m23"> m23{ *this };
		field<jfloat, "m30"> m30{ *this };
		field<jfloat, "m31"> m31{ *this };
		field<jfloat, "m32"> m32{ *this };
		field<jfloat, "m33"> m33{ *this };

		inline glm::mat4 to_glm_mat4()
		{
			return glm::mat4(m00.get(), m01.get(), m02.get(), m03.get(),
				m10.get(), m11.get(), m12.get(), m13.get(),
				m20.get(), m21.get(), m22.get(), m23.get(),
				m30.get(), m31.get(), m32.get(), m33.get()
			);
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Quaternionf, "org/joml/Quaternionf")
		constructor<> init{*this};
	END_KLASS_DEF()



	BEGIN_KLASS_DEF(Vec3d, "net/minecraft/world/phys/Vec3")
		field<jdouble, "x"> x{ *this };
		field<jdouble, "y"> y{ *this };
		field<jdouble, "z"> z{ *this };

		inline maths::vector3d to_vector3d()
		{
			return { x.get(), y.get(), z.get() };
		}

		inline glm::dvec3 to_glm_dvec3()
		{
			return { x.get(), y.get(), z.get() };
		}

		inline glm::vec3 to_glm_vec3()
		{
			return { x.get(), y.get(), z.get() };
		}

		inline void set_glm_dvec3(const glm::dvec3& vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Item, "net/minecraft/world/item/Item")
	END_KLASS_DEF()
	BEGIN_KLASS_DEF_EX(SwordItem, "net/minecraft/world/item/SwordItem", Item)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(ItemStack, "net/minecraft/world/item/ItemStack")
		field<Item, "item"> item{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Box, "net/minecraft/world/phys/AABB")
		field<jdouble, "minX"> minX{ *this };
		field<jdouble, "minY"> minY{ *this };
		field<jdouble, "minZ"> minZ{ *this };
		field<jdouble, "maxX"> maxX{ *this };
		field<jdouble, "maxY"> maxY{ *this };
		field<jdouble, "maxZ"> maxZ{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(EntityPose, "net/minecraft/world/entity/Pose")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(EntityDimensions, "net/minecraft/world/entity/EntityDimensions")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffect, "net/minecraft/world/effect/MobEffect")
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffects, "net/minecraft/world/effect/MobEffects")
		inline static static_field<StatusEffect, "GLOWING"> GLOWING{};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(StatusEffectInstance, "net/minecraft/world/effect/MobEffectInstance")
		constructor<StatusEffect, jint, jint> init{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Entity, "net/minecraft/world/entity/Entity")
		field<Box, "bb"> boundingBox{ *this };
		field<jint, "tickCount"> age{ *this };
		field<Vec3d, "position"> pos{ *this };
		field<jfloat, "yRot"> yaw{ *this };
		field<jfloat, "xRot"> pitch{ *this };
		field<jfloat, "yRotO"> prevYaw{ *this };
		field<jfloat, "xRotO"> prevPitch{ *this };
		field<jfloat, "fallDistance"> fallDistance{ *this };
		field<jboolean, "onGround"> onGround{ *this };

		field<jdouble, "xOld"> lastRenderX{ *this };
		field<jdouble, "yOld"> lastRenderY{ *this };
		field<jdouble, "zOld"> lastRenderZ{ *this };

		field<jfloat, "walkDist"> horizontalSpeed{ *this };
		field<jfloat, "walkDistO"> prevHorizontalSpeed{ *this };

		field<Vec3d, "deltaMovement"> velocity{ *this };

		inline maths::vector3d get_prev_position()
		{
			return { lastRenderX.get(), lastRenderY.get(), lastRenderZ.get() };
		}

		inline glm::vec3 get_last_render_glm_vec3_pos()
		{
			return { lastRenderX.get(), lastRenderY.get(), lastRenderZ.get() };
		}

		inline maths::vector3d get_position()
		{
			return pos.get().to_vector3d();
		}

		inline maths::angles get_prev_angles()
		{
			return { prevYaw.get(), prevPitch.get() };
		}

		inline maths::angles get_angles()
		{
			return { yaw.get(), pitch.get() };
		}

		inline glm::vec2 get_rotation_glm_vec2()
		{
			return { pitch.get(), yaw.get() };
		}
		inline glm::vec2 get_prev_rotation_glm_vec2()
		{
			return { pitch.get(), yaw.get() };
		}

		method<EntityPose, "getPose"> getPose{ *this };
		method<EntityDimensions, "getDimensions", EntityPose> getDimensions{ *this };
		method<jfloat, "getEyeHeight", EntityPose> getEyeHeight{ *this };
		method<jboolean, "isAlive"> isAlive{ *this };
		method<void, "setGlowingTag", jboolean> setGlowing{ *this };
		method<void, "setSharedFlag", jint, jboolean> setFlag{ *this };
		method<void, "setSprinting", jboolean> setSprinting{ *this };
		method<jboolean, "isSprinting"> isSprinting{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(LivingEntity, "net/minecraft/world/entity/LivingEntity", Entity)
		field<jint, "attackStrengthTicker"> lastAttackedTicks{ *this };
		field<jint, "hurtTime"> hurtTime{*this};

		method<jboolean, "isHolding", Item> isHolding{ *this };
		method<ItemStack, "getMainHandItem"> getMainHandStack{ *this };
		method<jboolean, "addEffect", StatusEffectInstance> addStatusEffect{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(PlayerEntity, "net/minecraft/world/entity/player/Player", LivingEntity)
		method<jfloat, "getAttackStrengthScale", jfloat> getAttackCooldownProgress{ *this };

		field<jfloat, "bob"> strideDistance{ *this };
		field<jfloat, "oBob"> prevStrideDistance{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(AbstractClientPlayerEntity, "net/minecraft/client/player/AbstractClientPlayer", PlayerEntity)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(ClientPlayerEntity, "net/minecraft/client/player/LocalPlayer", AbstractClientPlayerEntity)
		field<jfloat, "xBob"> renderPitch{ *this };
		field<jfloat, "yBob"> renderYaw{ *this };
		field<jfloat, "xBobO"> lastRenderPitch{ *this };
		field<jfloat, "yBobO"> lastRenderYaw{ *this };

		inline glm::vec2 get_last_render_rotation()
		{
			return { lastRenderPitch.get(), lastRenderYaw.get() };
		}

		inline glm::vec2 get_render_rotation()
		{
			return { renderPitch.get(), renderYaw.get() };
		}
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(BlockPos, "net/minecraft/core/BlockPos") // net/minecraft/util/math/BlockPos
		constructor<jint, jint, jint> init{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(BlockState, "net/minecraft/world/level/block/state/BlockState", Object) // net/minecraft/block/BlockState
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(World, "net/minecraft/world/level/Level") // net/minecraft/world/World
		method<BlockState, "getBlockState", BlockPos> getBlockState{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(ClientWorld, "net/minecraft/client/multiplayer/ClientLevel", World)
		field<List, "players"> players{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(SimpleOption, "net/minecraft/client/OptionInstance")
		field<Object, "value"> value{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(GameOptions, "net/minecraft/client/Options")
		field<SimpleOption, "sensitivity"> mouseSensitivity{ *this };
		field<SimpleOption, "bobView"> bobView{ *this };
		field<SimpleOption, "fov"> fov{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Mouse, "net/minecraft/client/MouseHandler")
		field<jdouble, "accumulatedDX"> cursorDeltaX{ *this };
		field<jdouble, "accumulatedDY"> cursorDeltaY{ *this };

		method<void, "turnPlayer"> updateMouse{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(RenderTickCounter, "net/minecraft/client/Timer")
		field<jfloat, "partialTick"> tickDelta{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(HitResult$Type, "net/minecraft/world/phys/HitResult$Type")
		inline static static_field<HitResult$Type, "ENTITY"> ENTITY{};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(HitResult, "net/minecraft/world/phys/HitResult")
		method<HitResult$Type, "getType"> getType{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(EntityHitResult, "net/minecraft/world/phys/EntityHitResult", HitResult)
		field<Entity, "entity"> entity{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Camera, "net/minecraft/client/Camera") // yarn-named: net/minecraft/client/render/Camera
		field<float, "xRot"> pitch{*this};
		field<float, "yRot"> yaw{ *this };

		field<Vec3d, "position"> pos{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(GameRenderer, "net/minecraft/client/renderer/GameRenderer") // yarn-named: net/minecraft/client/render/GameRenderer
		field<Camera, "mainCamera"> camera{*this};
		field<jfloat, "oldFov"> lastFovMultiplier{*this};
		field<jfloat, "fov"> fovMultiplier{ *this };

		method<Matrix4f, "getProjectionMatrix", jdouble> getBasicProjectionMatrix{*this};
		method<jdouble, "getFov", Camera, jfloat, jboolean> getFov{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(MinecraftClient, "net/minecraft/client/Minecraft")
		inline static static_field<MinecraftClient, "instance"> instance{};

		field<ClientWorld, "level"> world{ *this };
		field<ClientPlayerEntity, "player"> player{ *this };
		field<HitResult, "hitResult"> crosshairTarget{ *this };

		field<Mouse, "mouseHandler"> mouse{ *this };
		field<GameOptions, "options"> options{ *this };
		field<RenderTickCounter, "timer"> renderTickCounter{ *this };
		field<GameRenderer, "gameRenderer"> gameRenderer{ *this };
	END_KLASS_DEF()
}