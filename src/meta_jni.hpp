#pragma once

#ifdef _WIN32
	#include <Windows.h>
#elif defined(__linux__)
	#include <pthread.h>
#endif
#include <jni.h>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <memory>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <cstdint>

#define assertm(exp, msg) assert(((void)msg, exp))

#define BEGIN_KLASS_DEF(unobf_klass_name, obf_klass_name) struct unobf_klass_name##_members; using unobf_klass_name = jni::klass<obf_klass_name, unobf_klass_name##_members>; struct unobf_klass_name##_members : public jni::empty_members	{ unobf_klass_name##_members(jclass owner_klass, jobject object_instance, bool is_global_ref) : jni::empty_members(owner_klass, object_instance, is_global_ref) {}

#define END_KLASS_DEF()	};

#define BEGIN_KLASS_DEF_EX(unobf_klass_name, obf_klass_name, inherit_from) struct unobf_klass_name##_members; using unobf_klass_name = jni::klass<obf_klass_name, unobf_klass_name##_members>; struct unobf_klass_name##_members : public inherit_from##_members { unobf_klass_name##_members(jclass owner_klass, jobject object_instance, bool is_global_ref) : inherit_from##_members(owner_klass, object_instance, is_global_ref) {}


namespace jni
{
	inline uint32_t _tls_index = 0;
	inline std::vector<jobject> _refs_to_delete{};
	inline std::mutex _refs_to_delete_mutex{};

	inline JNIEnv* get_env()
	{
#ifdef _WIN32
		return (JNIEnv*)TlsGetValue(_tls_index);
#elif __linux__
		return (JNIEnv*)pthread_getspecific(_tls_index);
#endif
	}
	inline void set_thread_env(JNIEnv* new_env)
	{
		if (get_env()) return;
#ifdef _WIN32
		TlsSetValue(_tls_index, new_env);
#elif __linux__
		pthread_setspecific(_tls_index, new_env);
#endif
	}

	inline void init()
	{
		if (_tls_index) return;
#ifdef _WIN32
		_tls_index = TlsAlloc();
#elif __linux__
		pthread_key_create(&_tls_index, nullptr);
#endif
		assertm(_tls_index, "tls index allocation failed");
	}
	inline void shutdown() //needs to be called on exit, library unusable after this
	{
		{
			std::lock_guard lock{ _refs_to_delete_mutex }; //shouldn't be necessary, every jni calls should be stopped before calling jni::destroy_cache
			for (jobject object : _refs_to_delete)
			{
				if (!object) continue;
				get_env()->DeleteGlobalRef(object);
			}
		}
		
#ifdef _WIN32
		TlsFree(_tls_index);
#elif __linux__
		pthread_key_delete(_tls_index);
#endif
	}

	template<size_t N>
	struct string_litteral
	{
		constexpr string_litteral(const char(&str)[N])
		{
			std::copy_n(str, N, value);
		}
		constexpr operator const char* () const
		{
			return value;
		}
		constexpr operator std::string_view() const
		{
			return value;
		}
		char value[N];
	};

	template<string_litteral... strs> inline constexpr auto concat()
	{
		constexpr std::size_t size = ((sizeof(strs.value) - 1) + ...); //-1 to not include null terminator (dumb)
		char concatenated[size + 1] = { '\0' }; //+1 for null terminator

		auto append = [i = 0, &concatenated](auto const& s) mutable
		{
			for (int n = 0; n < sizeof(s.value) - 1; ++n) concatenated[i++] = s.value[n]; //-1 to not include null terminator
		};
		(append(strs), ...);
		concatenated[size] = '\0';
		return string_litteral(concatenated);
	}

	class object_wrapper
	{
	public:
		object_wrapper(jobject object_instance, bool is_global_ref) :
			object_instance((is_global_ref && object_instance ? get_env()->NewGlobalRef(object_instance) : object_instance)),
			is_global_ref(is_global_ref)
		{
		}

		object_wrapper(const object_wrapper& other) :
			object_wrapper(other.object_instance, other.is_global_ref)
		{
		}

		~object_wrapper()
		{
			if (is_global_ref)
				clear_ref();
		}

		object_wrapper& operator=(const object_wrapper& other) //operator = keeps the current ref type
		{
			if (is_global_ref)
			{
				clear_ref();
				object_instance = get_env()->NewGlobalRef(other.object_instance);
			}
			else
				object_instance = other.object_instance;
			return *this;
		}

		void clear_ref()
		{
			if (!object_instance) return;
			if (is_global_ref)
				get_env()->DeleteGlobalRef(object_instance);
			object_instance = nullptr;
		}

		operator jobject() const
		{
			return this->object_instance;
		}

		bool is_global() const
		{
			return is_global_ref;
		}

		jobject object_instance;
	private:
		bool is_global_ref; //global refs aren't destroyed on PopLocalFrame, and can be shared between threads
	};

	struct empty_members : public object_wrapper
	{
		empty_members(jclass owner_klass, jobject object_instance, bool is_global_ref) :
			object_wrapper(object_instance, is_global_ref),
			owner_klass(owner_klass)
		{
		}

		jclass owner_klass;
	};

	template<string_litteral class_name, class fields_type = empty_members> class klass;

	template<typename T, typename... U> inline constexpr bool is_any_of_type = (std::is_same_v<T, U> || ...);
	template<typename T> inline constexpr bool is_jni_primitive_type = is_any_of_type<T, jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble>;


	template<typename klass_type> struct jclass_cache
	{
		inline static std::shared_mutex mutex{};
		inline static jclass value = nullptr;
	};

	template<typename klass_type> inline jclass get_cached_jclass() //findClass
	{
		jclass& cached = jclass_cache<klass_type>::value;
		{
			std::shared_lock shared_lock{ jclass_cache<klass_type>::mutex };
			if (cached) return cached;
		}
		jclass found = (jclass)get_env()->NewGlobalRef(get_env()->FindClass(klass_type::get_name()));
		assertm(found, (const char*)(concat<"failed to find class: ", klass_type::get_name()>()));
		{
			std::unique_lock unique_lock{ jclass_cache<klass_type>::mutex };
			cached = found;
		}
		{
			std::lock_guard lock{ _refs_to_delete_mutex };
			_refs_to_delete.push_back(found);
		}
		return found;
	}

	enum is_static_t : bool
	{
		STATIC = true,
		NOT_STATIC = false
	};

	template<class T> inline constexpr auto get_signature_for_type()
	{
		if constexpr (std::is_void_v<T>) 
			return string_litteral("V");
		if constexpr (!is_jni_primitive_type<T> && !std::is_void_v<T>)
			return T::get_signature();
		if constexpr (std::is_same_v<jboolean, T>)
			return string_litteral("Z");
		if constexpr (std::is_same_v<jbyte, T>)
			return string_litteral("B");
		if constexpr (std::is_same_v<jchar, T>)
			return string_litteral("C");
		if constexpr (std::is_same_v<jshort, T>)
			return string_litteral("S");
		if constexpr (std::is_same_v<jint, T>)
			return string_litteral("I");
		if constexpr (std::is_same_v<jfloat, T>)
			return string_litteral("F");
		if constexpr (std::is_same_v<jlong, T>)
			return string_litteral("J");
		if constexpr (std::is_same_v<jdouble, T>)
			return string_litteral("D");
	}

	template<class array_element_type>
	class array : public object_wrapper
	{
	public:
		array(jobject object_instance, bool is_global_ref = false) :
			object_wrapper(object_instance, is_global_ref)
		{
		}

		std::vector<array_element_type> to_vector()
		{
			jsize length = get_length();
			std::vector<array_element_type> vector{};
			vector.reserve(length);
			if constexpr (!is_jni_primitive_type<array_element_type>)
			{
				for (jsize i = 0; i < length; ++i)
					vector.push_back( array_element_type(get_env()->GetObjectArrayElement((jobjectArray)object_instance, i)) );
			}
			if constexpr (std::is_same_v<jboolean, array_element_type>)
			{
				std::unique_ptr<jboolean[]> buffer = std::make_unique<jboolean[]>(length);
				get_env()->GetBooleanArrayRegion((jbooleanArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jbyte, array_element_type>)
			{
				std::unique_ptr<jbyte[]> buffer = std::make_unique<jbyte[]>(length);
				get_env()->GetByteArrayRegion((jbyteArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jchar, array_element_type>)
			{
				std::unique_ptr<jchar[]> buffer = std::make_unique<jchar[]>(length);
				get_env()->GetCharArrayRegion((jcharArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jshort, array_element_type>)
			{
				std::unique_ptr<jshort[]> buffer = std::make_unique<jshort[]>(length);
				get_env()->GetShortArrayRegion((jshortArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jint, array_element_type>)
			{
				std::unique_ptr<jint[]> buffer = std::make_unique<jint[]>(length);
				get_env()->GetIntArrayRegion((jintArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jfloat, array_element_type>)
			{
				std::unique_ptr<jfloat[]> buffer = std::make_unique<jfloat[]>(length);
				get_env()->GetFloatArrayRegion((jfloatArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jlong, array_element_type>)
			{
				std::unique_ptr<jlong[]> buffer = std::make_unique<jlong[]>(length);
				get_env()->GetLongArrayRegion((jlongArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jdouble, array_element_type>)
			{
				std::unique_ptr<jdouble[]> buffer = std::make_unique<jdouble[]>(length);
				get_env()->GetDoubleArrayRegion((jdoubleArray)object_instance, 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			return vector;
		}

		jsize get_length()
		{
			return get_env()->GetArrayLength((jarray)object_instance);
		}

		static constexpr auto get_signature()
		{
			return concat<"[", get_signature_for_type<array_element_type>()>();
		}

		static constexpr auto get_name() //this is used for FindClass
		{
			return get_signature();
		}

		static array create(const std::vector<array_element_type>& values)
		{
			jobject object = nullptr;
			if constexpr (!is_jni_primitive_type<array_element_type>)
			{
				object = get_env()->NewObjectArray((jsize)values.size(), get_cached_jclass<array_element_type>(), nullptr);
				for (jsize i = 0; i < values.size(); ++i)
					get_env()->SetObjectArrayElement((jobjectArray)object, i, (jobject)values[i]);
			}
			if constexpr (std::is_same_v<jboolean, array_element_type>)
			{
				object = get_env()->NewBooleanArray((jsize)values.size());
				get_env()->SetBooleanArrayRegion((jbooleanArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jbyte, array_element_type>)
			{
				object = get_env()->NewByteArray((jsize)values.size());
				get_env()->SetByteArrayRegion((jbyteArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jchar, array_element_type>)
			{
				object = get_env()->NewCharArray((jsize)values.size());
				get_env()->SetCharArrayRegion((jcharArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jshort, array_element_type>)
			{
				object = get_env()->NewShortArray((jsize)values.size());
				get_env()->SetShortArrayRegion((jshortArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jint, array_element_type>)
			{
				object = get_env()->NewIntArray((jsize)values.size());
				get_env()->SetIntArrayRegion((jintArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jfloat, array_element_type>)
			{
				object = get_env()->NewFloatArray((jsize)values.size());
				get_env()->SetFloatArrayRegion((jfloatArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jlong, array_element_type>)
			{
				object = get_env()->NewLongArray((jsize)values.size());
				get_env()->SetLongArrayRegion((jlongArray)object, 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jdouble, array_element_type>)
			{
				object = get_env()->NewDoubleArray((jsize)values.size());
				get_env()->SetDoubleArrayRegion((jdoubleArray)object, 0, (jsize)values.size(), values.data());
			}
			return array(object);
		}
	};

	template<typename field_type, string_litteral field_name, is_static_t is_static = NOT_STATIC>
	class field
	{
	public:
		field(const empty_members& m) :
			owner_klass(m.owner_klass),
			object_instance(m.object_instance)
		{
			if (id) return;
			if constexpr (is_static)
				id = get_env()->GetStaticFieldID(owner_klass, get_name(), get_signature());
			if constexpr (!is_static)
				id = get_env()->GetFieldID(owner_klass, get_name(), get_signature());
			assertm(id, (const char*)(concat<"failed to find fieldID: ", get_name(), " ", get_signature()>()));
		}

		field& operator=(const field_type& new_value)
		{
			set(new_value);
			return *this;
		}

		void set(const field_type& new_value)
		{
			if (!id || !owner_klass || (!is_static && !object_instance)) return;
			if constexpr (!is_jni_primitive_type<field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticObjectField(owner_klass, id, (jobject)new_value);
				if constexpr (!is_static)
					return get_env()->SetObjectField(object_instance, id, (jobject)new_value);
			}
			if constexpr (std::is_same_v<jboolean, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticBooleanField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetBooleanField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jbyte, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticByteField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetByteField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jchar, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticCharField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetCharField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jshort, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticShortField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetShortField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jint, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticIntField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetIntField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jfloat, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticFloatField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetFloatField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jlong, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticLongField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetLongField(object_instance, id, new_value);
			}
			if constexpr (std::is_same_v<jdouble, field_type>)
			{
				if constexpr (is_static)
					return get_env()->SetStaticDoubleField(owner_klass, id, new_value);
				if constexpr (!is_static)
					return get_env()->SetDoubleField(object_instance, id, new_value);
			}
		}

		auto get() //usable only if field is not static (requires an instance of the owner class
		{
			if constexpr (!is_jni_primitive_type<field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return field_type(nullptr);
				if constexpr (is_static)
					return field_type(get_env()->GetStaticObjectField(owner_klass, id));
				if constexpr (!is_static)
					return field_type(get_env()->GetObjectField(object_instance, id));
			}
			if constexpr (std::is_same_v<jboolean, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jboolean(JNI_FALSE);
				if constexpr (is_static)
					return get_env()->GetStaticBooleanField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetBooleanField(object_instance, id);
			}
			if constexpr (std::is_same_v<jbyte, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jbyte(0);
				if constexpr (is_static)
					return get_env()->GetStaticByteField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetByteField(object_instance, id);
			}
			if constexpr (std::is_same_v<jchar, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jchar(0);
				if constexpr (is_static)
					return get_env()->GetStaticCharField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetCharField(object_instance, id);
			}
			if constexpr (std::is_same_v<jshort, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jshort(0);
				if constexpr (is_static)
					return get_env()->GetStaticShortField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetShortField(object_instance, id);
			}
			if constexpr (std::is_same_v<jint, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jint(0);
				if constexpr (is_static)
					return get_env()->GetStaticIntField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetIntField(object_instance, id);
			}
			if constexpr (std::is_same_v<jfloat, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jfloat(0.f);
				if constexpr (is_static)
					return get_env()->GetStaticFloatField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetFloatField(object_instance, id);
			}
			if constexpr (std::is_same_v<jlong, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jlong(0LL);
				if constexpr (is_static)
					return get_env()->GetStaticLongField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetLongField(object_instance, id);
			}
			if constexpr (std::is_same_v<jdouble, field_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jdouble(0.0);
				if constexpr (is_static)
					return get_env()->GetStaticDoubleField(owner_klass, id);
				if constexpr (!is_static)
					return get_env()->GetDoubleField(object_instance, id);
			}
		}

		static constexpr auto get_name()
		{
			return field_name;
		}

		static constexpr auto get_signature()
		{
			return get_signature_for_type<field_type>();
		}

		static constexpr bool is_field_static()
		{
			return is_static;
		}

		operator jfieldID() const
		{
			return id;
		}
	private:
		jclass owner_klass;
		jobject object_instance;
		inline static jfieldID id = nullptr;
	};


	template<typename method_return_type, string_litteral method_name, is_static_t is_static = NOT_STATIC, class... method_parameters_type>
	class method
	{
	public:
		method(const empty_members& m) :
			owner_klass(m.owner_klass),
			object_instance(m.object_instance)
		{
			if (id) return;
			if constexpr (is_static)
				id = get_env()->GetStaticMethodID(owner_klass, get_name(), get_signature());
			if constexpr (!is_static)
				id = get_env()->GetMethodID(owner_klass, get_name(), get_signature());
			assertm(id, (const char*)(concat<"failed to find methodID: ", get_name(), " ", get_signature()>()));
		}

		auto operator()(const method_parameters_type&... method_parameters)
		{
			return call(method_parameters...);
		}

		auto call(const method_parameters_type&... method_parameters)
		{
			if constexpr (std::is_void_v<method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return;
				if constexpr (is_static)
					get_env()->CallStaticVoidMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					get_env()->CallVoidMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				return;
			}

			if constexpr (!is_jni_primitive_type<method_return_type> && !std::is_void_v<method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return method_return_type(nullptr);
				if constexpr (is_static)
					return method_return_type(get_env()->CallStaticObjectMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...));
				if constexpr (!is_static)
					return method_return_type(get_env()->CallObjectMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...));
			}
			if constexpr (std::is_same_v<jboolean, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jboolean(JNI_FALSE);
				if constexpr (is_static)
					return get_env()->CallStaticBooleanMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallBooleanMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jbyte, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jbyte(0);
				if constexpr (is_static)
					return get_env()->CallStaticByteMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallByteMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jchar, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jchar(0);
				if constexpr (is_static)
					return get_env()->CallStaticCharMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallCharMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jshort, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jshort(0);
				if constexpr (is_static)
					return get_env()->CallStaticShortMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallShortMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jint, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jint(0);
				if constexpr (is_static)
					return get_env()->CallStaticIntMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallIntMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jfloat, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jfloat(0.f);
				if constexpr (is_static)
					return get_env()->CallStaticFloatMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallFloatMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jlong, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jlong(0LL);
				if constexpr (is_static)
					return get_env()->CallStaticLongMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallLongMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jdouble, method_return_type>)
			{
				if (!id || !owner_klass || (!is_static && !object_instance)) return jdouble(0.0);
				if constexpr (is_static)
					return get_env()->CallStaticDoubleMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				if constexpr (!is_static)
					return get_env()->CallDoubleMethod(object_instance, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
		}


		operator jmethodID() const
		{
			return id;
		}

		static constexpr auto get_name()
		{
			return method_name;
		}

		static constexpr auto get_signature()
		{
			return concat<"(", get_signature_for_type<method_parameters_type>()..., ")", get_signature_for_type<method_return_type>()>();
		}

		static constexpr bool is_method_static()
		{
			return is_static;
		}

	private:
		jclass owner_klass;
		jobject object_instance;
		inline static jmethodID id;
	};


	template<class... method_parameters_type>
	using constructor = method<void, "<init>", jni::NOT_STATIC, method_parameters_type...>;

	/*
	template<class T> struct remove_member_pointer
	{
		typedef T type;
	};
	template<class C, class T> struct remove_member_pointer<T C::*>
	{
		typedef T type;
	};
	*/


	template<string_litteral class_name, class members_type>
	class klass : public members_type
	{
	public:
		klass(jobject object_instance = nullptr, bool is_global_ref = false) :
			members_type(get_cached_jclass<klass>(), object_instance, is_global_ref) //be careful order of initialization matters
		{
		}
		
		template<class... method_parameters_type>
		static klass new_object(jni::constructor<method_parameters_type...> klass::*constructor, const method_parameters_type&... method_parameters)
		{
			klass tmp{}; //lmao
			return klass{jni::get_env()->NewObject(get_cached_jclass<klass>(), jmethodID(tmp.*constructor), std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...)};
		}

		static constexpr auto get_name()
		{
			return class_name;
		}

		static constexpr auto get_signature()
		{
			return concat<"L", class_name, ";">();
		}
	};
}