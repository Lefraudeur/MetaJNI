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
		jni::method<String, "toString"> toString{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(Class, "java/lang/Class")
		operator jclass() const
		{
			return (jclass)object_instance;
		}
	END_KLASS_DEF()

	BEGIN_KLASS_MEMBERS(String)
		inline static String create(const char* str)
		{
			return String(jni::get_env()->NewStringUTF(str));
		}


		inline std::string to_string() const
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
		jni::method<Class, "loadClass", jni::NOT_STATIC, String> loadClass{ *this };
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
	BEGIN_KLASS_DEF(Boolean, "java/lang/Boolean")
		jni::method<jboolean, "booleanValue"> booleanValue{ *this };
	END_KLASS_DEF()
	BEGIN_KLASS_DEF(Integer, "java/lang/Integer")
		jni::method<jint, "intValue"> intValue{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(PrintStream, "java/io/PrintStream")
		jni::method<void, "println", jni::NOT_STATIC, String> println{ *this };
	END_KLASS_DEF()

		BEGIN_KLASS_DEF(System, "java/lang/System")
		jni::field<PrintStream, "out", jni::STATIC> out{ *this };
	END_KLASS_DEF()


	// minecraft external libraries, also shouldn't change

	BEGIN_KLASS_DEF(Matrix4f, "org/joml/Matrix4f")
		jni::field<jfloat, "m00"> m00{*this};
		jni::field<jfloat, "m01"> m01{ *this };
		jni::field<jfloat, "m02"> m02{ *this };
		jni::field<jfloat, "m03"> m03{ *this };
		jni::field<jfloat, "m10"> m10{ *this };
		jni::field<jfloat, "m11"> m11{ *this };
		jni::field<jfloat, "m12"> m12{ *this };
		jni::field<jfloat, "m13"> m13{ *this };
		jni::field<jfloat, "m20"> m20{ *this };
		jni::field<jfloat, "m21"> m21{ *this };
		jni::field<jfloat, "m22"> m22{ *this };
		jni::field<jfloat, "m23"> m23{ *this };
		jni::field<jfloat, "m30"> m30{ *this };
		jni::field<jfloat, "m31"> m31{ *this };
		jni::field<jfloat, "m32"> m32{ *this };
		jni::field<jfloat, "m33"> m33{ *this };

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
		jni::constructor<> constructor{*this};
	END_KLASS_DEF()



	BEGIN_KLASS_DEF(Vec3d, "net/minecraft/world/phys/Vec3")
		jni::field<jdouble, "x"> x{ *this };
		jni::field<jdouble, "y"> y{ *this };
		jni::field<jdouble, "z"> z{ *this };

		inline maths::vector3d to_vector3d()
		{
			return { x.get(), y.get(), z.get() };
		}

		inline glm::vec3 to_glm_vec3()
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

		jni::field<jfloat, "walkDist"> horizontalSpeed{ *this };
		jni::field<jfloat, "walkDistO"> prevHorizontalSpeed{ *this };


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

		jni::field<jfloat, "bob"> strideDistance{ *this };
		jni::field<jfloat, "oBob"> prevStrideDistance{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(AbstractClientPlayerEntity, "net/minecraft/client/player/AbstractClientPlayer", PlayerEntity)
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(ClientPlayerEntity, "net/minecraft/client/player/LocalPlayer", AbstractClientPlayerEntity)
		jni::field<jfloat, "xBob"> renderPitch{ *this };
		jni::field<jfloat, "yBob"> renderYaw{ *this };
		jni::field<jfloat, "xBobO"> lastRenderPitch{ *this };
		jni::field<jfloat, "yBobO"> lastRenderYaw{ *this };

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
		jni::constructor<jint, jint, jint> constructor{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(BlockState, "net/minecraft/world/level/block/state/BlockState", Object) // net/minecraft/block/BlockState
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(World, "net/minecraft/world/level/Level") // net/minecraft/world/World
		jni::method<BlockState, "getBlockState", jni::NOT_STATIC, BlockPos> getBlockState{*this};
	END_KLASS_DEF()

	BEGIN_KLASS_DEF_EX(ClientWorld, "net/minecraft/client/multiplayer/ClientLevel", World)
		jni::field<List, "players"> players{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(SimpleOption, "net/minecraft/client/OptionInstance")
		jni::field<Object, "value"> value{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(GameOptions, "net/minecraft/client/Options")
		jni::field<SimpleOption, "sensitivity"> mouseSensitivity{ *this };
		jni::field<SimpleOption, "bobView"> bobView{ *this };
		jni::field<SimpleOption, "fov"> fov{ *this };
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

	BEGIN_KLASS_DEF(Camera, "net/minecraft/client/Camera") // yarn-named: net/minecraft/client/render/Camera
		jni::field<float, "xRot"> pitch{*this};
		jni::field<float, "yRot"> yaw{ *this };

		jni::field<Vec3d, "position"> pos{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(GameRenderer, "net/minecraft/client/renderer/GameRenderer") // yarn-named: net/minecraft/client/render/GameRenderer
		jni::field<Camera, "mainCamera"> camera{*this};
		jni::field<jfloat, "oldFov"> lastFovMultiplier{*this};
		jni::field<jfloat, "fov"> fovMultiplier{ *this };

		jni::method<Matrix4f, "getProjectionMatrix", jni::NOT_STATIC, jdouble> getBasicProjectionMatrix{*this};
		jni::method<jdouble, "getFov", jni::NOT_STATIC, Camera, jfloat, jboolean> getFov{ *this };
	END_KLASS_DEF()

	BEGIN_KLASS_DEF(MinecraftClient, "net/minecraft/client/Minecraft")
		jni::field<MinecraftClient, "instance", jni::STATIC> instance{ *this };

		jni::field<ClientWorld, "level"> world{ *this };
		jni::field<ClientPlayerEntity, "player"> player{ *this };
		jni::field<HitResult, "hitResult"> crosshairTarget{ *this };

		jni::field<Mouse, "mouseHandler"> mouse{ *this };
		jni::field<GameOptions, "options"> options{ *this };
		jni::field<RenderTickCounter, "timer"> renderTickCounter{ *this };
		jni::field<GameRenderer, "gameRenderer"> gameRenderer{ *this };
	END_KLASS_DEF()
}