#pragma once

#ifdef _WIN32
	#include <Windows.h>
#elif defined(__linux__)
	#include <pthread.h>
#endif
#include <jni.h>
#include <string_view>
#include <type_traits>
#include <memory>
#include <vector>
#include <cstdint>
#include <mutex>
#include <functional>
#include <atomic>
#include <algorithm>
#include <utility>

#ifdef NDEBUG
	#define assertm(exp, msg) ;
#else
	#include <iostream>
	#define assertm(exp, msg) if (!(exp)) { std::cout << (msg) << '\n'; abort(); }
#endif

#define KLASS_DECLARATION(unobf_klass_name, obf_klass_name) \
struct unobf_klass_name##_members; \
using unobf_klass_name = jni::klass<obf_klass_name, unobf_klass_name##_members>;

#define BEGIN_KLASS_MEMBERS_EX(unobf_klass_name, inherit_from) \
struct unobf_klass_name##_members : public inherit_from##_members \
{ \
	template <typename field_type, jni::string_litteral field_name> \
	using field = jni::field<unobf_klass_name, field_type, field_name>; \
	\
	template <typename field_type, jni::string_litterals field_names> \
	using multi_field = jni::multi_field<unobf_klass_name, field_type, field_names>; \
    \
	template <typename field_type, jni::string_litteral field_name> \
	using static_field = jni::static_field<unobf_klass_name, field_type, field_name>; \
	\
	template <typename field_type, jni::string_litterals field_names> \
	using multi_static_field = jni::multi_static_field<unobf_klass_name, field_type, field_names>; \
	\
    \
	template <typename method_return_type, jni::string_litteral method_name, class... method_parameters_type> \
	using method = jni::method<unobf_klass_name, method_return_type, method_name, method_parameters_type...>; \
    \
	template <typename method_return_type, jni::string_litterals method_names, class... method_parameters_type> \
	using multi_method = jni::multi_method<unobf_klass_name, method_return_type, method_names, method_parameters_type...>; \
    \
	template <typename method_return_type, jni::string_litteral method_name, class... method_parameters_type> \
	using static_method = jni::static_method<unobf_klass_name, method_return_type, method_name, method_parameters_type...>; \
	\
	template <typename method_return_type, jni::string_litterals method_names, class... method_parameters_type> \
	using multi_static_method = jni::multi_static_method<unobf_klass_name, method_return_type, method_names, method_parameters_type...>; \
    \
	template <class... method_parameters_type> \
	using constructor = jni::constructor<unobf_klass_name, method_parameters_type...>; \
	\
	\
	unobf_klass_name##_members(const jni::object_wrapper& o_wrapper) : \
		inherit_from##_members(o_wrapper) \
	{ \
	} \
	\
	unobf_klass_name##_members(jni::object_wrapper&& o_wrapper) : \
		inherit_from##_members(std::move(o_wrapper)) \
	{ \
	}

#define BEGIN_KLASS_DEF_EX(unobf_klass_name, obf_klass_name, inherit_from) \
KLASS_DECLARATION(unobf_klass_name, obf_klass_name) \
BEGIN_KLASS_MEMBERS_EX(unobf_klass_name, inherit_from)

#define BEGIN_KLASS_MEMBERS(unobf_klass_name) BEGIN_KLASS_MEMBERS_EX(unobf_klass_name, jni::empty)
#define BEGIN_KLASS_DEF(unobf_klass_name, obf_klass_name) BEGIN_KLASS_DEF_EX(unobf_klass_name, obf_klass_name, jni::empty)

#define END_KLASS_DEF()	};
#define END_KLASS_MEMBERS()	};


namespace jni
{
	inline uint32_t _tls_index = 0;
	inline std::vector<jobject> _refs_to_delete{};
	inline std::mutex _refs_to_delete_mutex{};
	inline std::function<jclass(const char* class_name)> _custom_find_class{};

	enum reference_type
	{
		LOCAL,
		GLOBAL
	};

	inline JNIEnv* get_env()
	{
		if (!_tls_index) return nullptr;
#ifdef _WIN32
		return (JNIEnv*)TlsGetValue(_tls_index);
#elif __linux__
		return (JNIEnv*)pthread_getspecific(_tls_index);
#endif
	}
	inline void set_thread_env(JNIEnv* new_env)
	{
		if (get_env()) return;
		assertm(new_env, "called jni::set_thread_env with null new_env");
#ifdef _WIN32
		TlsSetValue(_tls_index, new_env);
#elif __linux__
		pthread_setspecific(_tls_index, new_env);
#endif
	}

	inline bool init()
	{
		if (_tls_index) return true;
#ifdef _WIN32
		_tls_index = TlsAlloc();
#elif __linux__
		pthread_key_create(&_tls_index, nullptr);
#endif
		assertm(_tls_index, "tls index allocation failed");
		if (!_tls_index) return false;
		return true;
	}
	inline void shutdown() //needs to be called on exit, library unusable after this
	{
		if (!get_env()) return;
		{
			std::lock_guard lock{ _refs_to_delete_mutex }; //shouldn't be necessary, every jni calls should be stopped before calling jni::destroy_cache
			for (jobject object : _refs_to_delete)
			{
				if (!object) continue;
				get_env()->DeleteGlobalRef(object);
			}
			_custom_find_class = {}; // destroy in case the custom find class stores a classloader reference
		}

#ifdef _WIN32
		TlsFree(_tls_index);
#elif __linux__
		pthread_key_delete(_tls_index);
#endif
	}

	// custom_find_class is expected to return a jclass (local reference)
	inline void set_custom_find_class(std::function<jclass(const char* class_name)> find_class)
	{
		_custom_find_class = find_class;
	}



	template<size_t N>
	struct string_litteral
	{
		consteval string_litteral(const char(&str)[N])
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

	template<string_litteral... strs> inline consteval auto concat()
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

	template<typename T> struct is_string_litteral : public std::false_type {};
	template<size_t N> struct is_string_litteral<string_litteral<N>> : public std::true_type {};

	template<typename T> concept string_litteral_t = is_string_litteral<T>::value;





	// recursive tuple implementation, because std::tuple is not a structural type
	template<size_t i, typename T> struct tuple_litteral_leaf
	{
		consteval tuple_litteral_leaf(const T& v) : value(v) {}
		T value;
	};

	template<std::size_t i, typename... types>
	struct tuple_litteral_impl;

	template<size_t i> struct tuple_litteral_impl<i> // base type, ends recursion, specialization of tuple_litteral_impl with empty types param pack
	{
	};

	template<size_t i, typename head, typename... others> struct tuple_litteral_impl<i, head, others...> :  // specialisation of tuple_litteral_impl, non empty types param pack
		public tuple_litteral_leaf<i, head>,
		// first type of "others" parameter pack, becomes head, and remaining types in the pack become "others",
		// recursion until others is empty and we inherit from base type tuple_litteral_impl<length> (specialized to end recursion)
		public tuple_litteral_impl<i + 1, others...>
	{
		consteval tuple_litteral_impl(const head& h, const others&... o) :
			tuple_litteral_leaf<i, head>(h),
			tuple_litteral_impl<i + 1, others...>(o...)
		{
		}
	};

	template<typename... items>
	using tuple_litteral = tuple_litteral_impl<0, items...>;

	template<typename T>
	struct tuple_litteral_transform_type
	{
		using type = T;
	};

	template<size_t N>
	struct tuple_litteral_transform_type<char[N]>
	{
		using type = string_litteral<N>;
	};

	template<typename T>
	using tuple_litteral_transform_type_t = typename tuple_litteral_transform_type<T>::type;

	// deduction guide, if constructor param of type const T&, instantiate tuple_litteral with type T, 
	// special case if T is char[N], in which case use string_litteral<N>
	template<typename... items>
	tuple_litteral_impl(const items&...) -> tuple_litteral<tuple_litteral_transform_type_t<items>...>;

	template<typename... items> consteval size_t tuple_litteral_size(const tuple_litteral<items...>& tuple)
	{
		return sizeof...(items);
	}

	template<size_t i, typename head, typename... others>
	consteval const head& tuple_litteral_get(const tuple_litteral_impl<i, head, others...>& tuple)
	{
		return tuple.tuple_litteral_leaf<i, head>::value;
	};

	template<string_litteral_t... string_litteral_ts>
	using string_litterals = tuple_litteral<string_litteral_ts...>;

	template<tuple_litteral tuple, size_t... is>
	// std::index_sequence parameter used just to deduce is to 0,1,2,3,...
	constexpr void tuple_litteral_foreach_impl(const auto& callable, std::index_sequence<is...>)
	{
		(callable.template operator() < is > (tuple_litteral_get<is>(tuple)), ...);
	}

	template<tuple_litteral tuple>
	constexpr void tuple_litteral_foreach(const auto& callable)
	{
		tuple_litteral_foreach_impl<tuple>(callable, std::make_index_sequence<tuple_litteral_size(tuple)>{});
	}

	template<size_t i, size_t tot_size>
	consteval auto space_or_empty_string_litteral()
	{
		if constexpr (i == tot_size - 1) return string_litteral("");
		else return string_litteral(" ");
	}

	template<string_litteral_t... items, size_t... is>
	consteval auto string_litterals_join_impl(string_litterals<items...> tuple, std::index_sequence<is...>)
	{
		constexpr size_t tuple_size = sizeof...(items);
		constexpr size_t size = ((sizeof(tuple_litteral_get<is>(tuple).value) - 1) + ...) + tuple_size - 1;
		char result[size + 1] = { '\0' };
		auto foreach = [i = 0, &result](const auto& s, bool space) mutable
			{
				for (int n = 0; n < sizeof(s.value) - 1; ++n)
					result[i++] = s.value[n];
				if (space)
					result[i++] = ' ';
			};
		(foreach(tuple_litteral_get<is>(tuple), (is == tuple_size - 1 ? false : true)), ...);

		result[size] = '\0';
		return string_litteral(result);
	}
	template<string_litteral_t... items>
	consteval auto string_litterals_join(string_litterals<items...> tuple)
	{
		return string_litterals_join_impl(tuple, std::make_index_sequence<sizeof...(items)>{});
	}

	template<tuple_litteral tuple, size_t... is>
	consteval auto tuple_litteral_map_impl(std::index_sequence<is...>, auto lambda)
	{
		return tuple_litteral{ lambda.template operator()<is, tuple_litteral_get<is>(tuple) > ()... };
	}

	template<tuple_litteral tuple>
	consteval auto tuple_litteral_map(auto lambda)
	{
		return tuple_litteral_map_impl<tuple>(std::make_index_sequence<tuple_litteral_size(tuple)>{}, lambda);
	}

	template<size_t i, typename... items> consteval auto tuple_litteral_get_or_last(const tuple_litteral<items...>& tuple)
	{
		if constexpr (i >= sizeof...(items))
			return tuple_litteral_get<sizeof...(items) - 1>(tuple);
		else
			return tuple_litteral_get<i>(tuple);
	}

	template<size_t... sizes> consteval size_t get_max()
	{
		size_t max = 0;
		([&max](size_t size)
		{
			if (size > max) max = size;
		}(sizes), ...);
		return max;
	}

	template<tuple_litteral... tuples> consteval size_t tuple_litteral_get_max_size()
	{
		return get_max<tuple_litteral_size(tuples)...>();
	}




	template<typename klass_type> struct jclass_cache
	{
		inline static std::atomic<jclass> value = nullptr;
	};

	template<typename klass_type> inline jclass get_cached_jclass() //findClass
	{
		JNIEnv* env = get_env();
		if (!env) return nullptr;

		std::atomic<jclass>& cached = jclass_cache<klass_type>::value;
		if (jclass(cached)) return jclass(cached);


		constexpr string_litterals class_names = klass_type::get_names();

		jclass found = nullptr;
		tuple_litteral_foreach<class_names>([&found, env]<size_t i>(const auto& class_name)
		{
			if (found) return;
			found = env->FindClass(class_name);
			if (env->ExceptionCheck())
				env->ExceptionClear();
			if (!found && _custom_find_class)
				found = (jclass)_custom_find_class(class_name);
		});


		assertm(found, (std::string_view)(concat<"failed to find class: ", klass_type::get_descriptive_name()>()));
		if (!found) return nullptr;

		found = (jclass)env->NewGlobalRef(found);
		{
			std::lock_guard lock{ _refs_to_delete_mutex };
			_refs_to_delete.push_back(found);
		}

		cached = found;
		return found;
	}


	class object_wrapper
	{
	public:
		object_wrapper(reference_type ref_type = reference_type::LOCAL) :
			ref_type(ref_type),
			object_instance(nullptr)
		{
		}

		object_wrapper(jobject object_instance, reference_type ref_type = reference_type::LOCAL) :
			ref_type(ref_type),
			object_instance((is_global() && object_instance ? get_env()->NewGlobalRef(object_instance) : object_instance))
		{
		}

		object_wrapper(const object_wrapper& other) :
			object_wrapper(other.object_instance, other.ref_type)
		{
		}

		object_wrapper(object_wrapper&& other) noexcept :
			object_instance(other.object_instance),
			ref_type(other.ref_type)
		{
			other.object_instance = nullptr;
		}

		virtual ~object_wrapper()
		{
			if (is_global())
				clear_ref();
		}

		object_wrapper& operator=(const object_wrapper& other) //operator = keeps the current ref type
		{
			if (is_global())
			{
				jobject old_instance = object_instance; // set before deleting, eg if operator= is called on itself or on an object_wrapper with the same object_instance
				object_instance = (other.object_instance ? get_env()->NewGlobalRef(other.object_instance) : nullptr);
				if (old_instance) get_env()->DeleteGlobalRef(old_instance);
			}
			else
				object_instance = other.object_instance;
			return *this;
		}

		object_wrapper& operator=(object_wrapper&& other) noexcept
		{
			if (!is_global())
			{
				object_instance = other.object_instance;
				return *this;
			}

			jobject old_instance = object_instance;
			if (other.is_global())
			{
				object_instance = other.object_instance;
				other.object_instance = nullptr;
			}
			else
				object_instance = (other.object_instance ? get_env()->NewGlobalRef(other.object_instance) : nullptr);
			if (old_instance) get_env()->DeleteGlobalRef(old_instance);

			return *this;
		}

		bool operator==(const object_wrapper& other) const
		{
			return is_same_object(other);
		}

		bool is_same_object(const object_wrapper& other) const
		{
			return get_env()->IsSameObject(object_instance, other.object_instance) == JNI_TRUE;
		}

		template<typename klass_type>
		bool is_instance_of() const
		{
			assertm(object_instance, (std::string_view)(concat<"called is_instance_of<", klass_type::get_name(), ">() with invalid object_instance">()));
			return get_env()->IsInstanceOf(object_instance, get_cached_jclass<klass_type>()) == JNI_TRUE;
		}

		void clear_ref()
		{
			if (!object_instance) return;
			if (is_global() && get_env())
				get_env()->DeleteGlobalRef(object_instance);
			object_instance = nullptr;
		}

		explicit operator jobject() const // made explcit to avoid ambiguity with the constructors that take a jobject, prefer using the object_wrapper one, 
		{
			return this->object_instance;
		}

		operator bool() const
		{
			return this->object_instance;
		}

		jobject get_jobject() const
		{
			return this->object_instance;
		}

		bool is_global() const
		{
			return ref_type == reference_type::GLOBAL;
		}

		reference_type get_reference_type() const
		{
			return reference_type::GLOBAL;
		}
		
	private:
		// warning order matters; ref_type must be initialized before object_instance
		reference_type ref_type; //global refs aren't destroyed on PopLocalFrame, and can be shared between threads
		jobject object_instance;
	};

	template<typename T, typename... U> inline constexpr bool is_any_of_type = (std::is_same_v<T, U> || ...);
	template<typename T> inline constexpr bool is_jni_primitive_type = is_any_of_type<T, jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble>;

	struct empty_members : public object_wrapper
	{
		empty_members(const empty_members& other) = delete; // we must never copy the jni::field / jni::method, as they hold a reference to *this
		empty_members(empty_members&& other) = delete;

		empty_members(const object_wrapper& o_wrapper) :
			object_wrapper(o_wrapper)
		{
		}

		empty_members(object_wrapper&& o_wrapper) :
			object_wrapper(std::move(o_wrapper))
		{
		}
	};

	// T should be jni::array or jni::klass
	template<class T> inline consteval auto get_signatures_for_type()
	{
		if constexpr (std::is_void_v<T>)
			return string_litterals("V");
		if constexpr (!is_jni_primitive_type<T> && !std::is_void_v<T>)
			return T::get_signatures();
		if constexpr (std::is_same_v<jboolean, T>)
			return string_litterals("Z");
		if constexpr (std::is_same_v<jbyte, T>)
			return string_litterals("B");
		if constexpr (std::is_same_v<jchar, T>)
			return string_litterals("C");
		if constexpr (std::is_same_v<jshort, T>)
			return string_litterals("S");
		if constexpr (std::is_same_v<jint, T>)
			return string_litterals("I");
		if constexpr (std::is_same_v<jfloat, T>)
			return string_litterals("F");
		if constexpr (std::is_same_v<jlong, T>)
			return string_litterals("J");
		if constexpr (std::is_same_v<jdouble, T>)
			return string_litterals("D");
	}

	template<class array_element_type>
	class array : public object_wrapper
	{
	public:
		explicit array(reference_type ref_type = reference_type::LOCAL) : array(jni::object_wrapper{ ref_type }) {}
		explicit array(const object_wrapper& other) : object_wrapper(other) {};
		explicit array(object_wrapper&& other) : object_wrapper(std::move(other)) {};

		array(const array& other) : array((const object_wrapper&)other) {}
		array(array&& other) noexcept : array(static_cast<object_wrapper&&>(other)) {}

		array& operator=(const array& other)
		{
			object_wrapper::operator=(other);
			return *this;
		}

		array& operator=(array&& other) noexcept
		{
			object_wrapper::operator=(std::move(other));
			return *this;
		}

		array new_global_ref() const
		{
			return array(this->get_jobject(), true);
		}

		static consteval auto get_signatures()
		{
			return tuple_litteral_map<get_signatures_for_type<array_element_type>()>(
			[]<size_t i, auto signature>()
			{
				return concat<"[", signature>();
			});
		}

		static consteval auto get_names() //this is used by get_cached_jclass
		{
			return get_signatures();
		}

		static consteval auto get_descriptive_name()
		{
			return string_litterals_join(get_names());
		}

		void set_elements(const std::vector<array_element_type>& values) const
		{
			assertm(this->get_jobject(), (std::string_view)(concat<"called jni::array::set_elements with invalid object_instance: ", get_descriptive_name()>()));
			if (!this->get_jobject()) return;
			if (!values.size()) return;
			if constexpr (!is_jni_primitive_type<array_element_type>)
			{
				for (jsize i = 0; i < values.size(); ++i)
					get_env()->SetObjectArrayElement((jobjectArray)this->get_jobject(), i, (jobject)values[i]);
			}
			if constexpr (std::is_same_v<jboolean, array_element_type>)
			{
				get_env()->SetBooleanArrayRegion((jbooleanArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jbyte, array_element_type>)
			{
				get_env()->SetByteArrayRegion((jbyteArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jchar, array_element_type>)
			{
				get_env()->SetCharArrayRegion((jcharArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jshort, array_element_type>)
			{
				get_env()->SetShortArrayRegion((jshortArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jint, array_element_type>)
			{
				get_env()->SetIntArrayRegion((jintArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jfloat, array_element_type>)
			{
				get_env()->SetFloatArrayRegion((jfloatArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jlong, array_element_type>)
			{
				get_env()->SetLongArrayRegion((jlongArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
			if constexpr (std::is_same_v<jdouble, array_element_type>)
			{
				get_env()->SetDoubleArrayRegion((jdoubleArray)this->get_jobject(), 0, (jsize)values.size(), values.data());
			}
		}

		std::vector<array_element_type> to_vector() const
		{
			jsize length = get_length();
			std::vector<array_element_type> vector{};
			if (!length) return vector;
			vector.reserve(length);
			if constexpr (!is_jni_primitive_type<array_element_type>)
			{
				for (jsize i = 0; i < length; ++i)
					vector.push_back(array_element_type(get_env()->GetObjectArrayElement((jobjectArray)this->get_jobject(), i)));
			}
			if constexpr (std::is_same_v<jboolean, array_element_type>)
			{
				std::unique_ptr<jboolean[]> buffer = std::make_unique<jboolean[]>(length);
				get_env()->GetBooleanArrayRegion((jbooleanArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jbyte, array_element_type>)
			{
				std::unique_ptr<jbyte[]> buffer = std::make_unique<jbyte[]>(length);
				get_env()->GetByteArrayRegion((jbyteArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jchar, array_element_type>)
			{
				std::unique_ptr<jchar[]> buffer = std::make_unique<jchar[]>(length);
				get_env()->GetCharArrayRegion((jcharArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jshort, array_element_type>)
			{
				std::unique_ptr<jshort[]> buffer = std::make_unique<jshort[]>(length);
				get_env()->GetShortArrayRegion((jshortArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jint, array_element_type>)
			{
				std::unique_ptr<jint[]> buffer = std::make_unique<jint[]>(length);
				get_env()->GetIntArrayRegion((jintArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jfloat, array_element_type>)
			{
				std::unique_ptr<jfloat[]> buffer = std::make_unique<jfloat[]>(length);
				get_env()->GetFloatArrayRegion((jfloatArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jlong, array_element_type>)
			{
				std::unique_ptr<jlong[]> buffer = std::make_unique<jlong[]>(length);
				get_env()->GetLongArrayRegion((jlongArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			if constexpr (std::is_same_v<jdouble, array_element_type>)
			{
				std::unique_ptr<jdouble[]> buffer = std::make_unique<jdouble[]>(length);
				get_env()->GetDoubleArrayRegion((jdoubleArray)this->get_jobject(), 0, length, buffer.get());
				vector.insert(vector.begin(), buffer.get(), buffer.get() + length);
			}
			return vector;
		}

		jsize get_length() const
		{
			assertm(this->get_jobject(), (std::string_view)(concat<"called jni::array::get_length() with invalid object_instance: ", get_descriptive_name()>()));
			if (!this->get_jobject())
				return 0;
			return get_env()->GetArrayLength((jarray)this->get_jobject());
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

	// o_klass is a jni::klass, used to know to what class this field belongs to
	template<typename o_klass, typename field_type, string_litterals field_names>
	class multi_static_field
	{
	public:

		static consteval auto get_names()
		{
			return field_names;
		}

		static consteval auto get_signatures()
		{
			return get_signatures_for_type<field_type>();
		}

		static consteval auto get_descriptive_name()
		{
			return concat< string_litterals_join(field_names), " : ", string_litterals_join(get_signatures()) >();
		}

		static void init_id()
		{
			if (jfieldID(id)) return;
			jclass owner_klass = get_cached_jclass<o_klass>();
			jfieldID new_id = nullptr;
			if (owner_klass)
			{
				tuple_litteral_foreach<field_names>([&new_id, owner_klass]<size_t i>(const auto& field_name)
				{
					if (new_id) return;
					constexpr string_litterals signatures = get_signatures();
					new_id = get_env()->GetStaticFieldID(owner_klass, field_name, tuple_litteral_get_or_last<i>(signatures));
				});

				if (new_id) id = new_id;
			}
			assertm(new_id, (std::string_view)(concat<"failed to find fieldID: ", get_descriptive_name()>()));
		}

		operator jfieldID() const
		{
			init_id();
			return id;
		}

		multi_static_field() = default;

		multi_static_field(const multi_static_field& other) = delete; // make sure field won't be copied (we store a empty_members reference which must not be copied)
		multi_static_field(multi_static_field&& other) = delete;

		multi_static_field& operator=(const field_type& new_value)
		{
			set(new_value);
			return *this;
		}

		void set(const field_type& new_value)
		{
			init_id();
			jclass owner_klass = get_cached_jclass<o_klass>();
			if (!jfieldID(id) || !owner_klass) return;
			if constexpr (!is_jni_primitive_type<field_type>)
				return get_env()->SetStaticObjectField(owner_klass, id, (jobject)new_value);

			if constexpr (std::is_same_v<jboolean, field_type>)
				return get_env()->SetStaticBooleanField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jbyte, field_type>)
				return get_env()->SetStaticByteField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jchar, field_type>)
				return get_env()->SetStaticCharField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jshort, field_type>)
				return get_env()->SetStaticShortField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jint, field_type>)
				return get_env()->SetStaticIntField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jfloat, field_type>)
				return get_env()->SetStaticFloatField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jlong, field_type>)
				return get_env()->SetStaticLongField(owner_klass, id, new_value);

			if constexpr (std::is_same_v<jdouble, field_type>)
				return get_env()->SetStaticDoubleField(owner_klass, id, new_value);
		}

		auto get() const
		{
			init_id();
			jclass owner_klass = get_cached_jclass<o_klass>();
			bool not_valid = !jfieldID(id) || !owner_klass;
			if constexpr (!is_jni_primitive_type<field_type>)
			{
				if (not_valid) return field_type(nullptr);
				return field_type(get_env()->GetStaticObjectField(owner_klass, id));
			}
			if constexpr (std::is_same_v<jboolean, field_type>)
			{
				if (not_valid) return jboolean(JNI_FALSE);
				return get_env()->GetStaticBooleanField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jbyte, field_type>)
			{
				if (not_valid) return jbyte(0);
				return get_env()->GetStaticByteField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jchar, field_type>)
			{
				if (not_valid) return jchar(0);
				return get_env()->GetStaticCharField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jshort, field_type>)
			{
				if (not_valid) return jshort(0);
				return get_env()->GetStaticShortField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jint, field_type>)
			{
				if (not_valid) return jint(0);
				return get_env()->GetStaticIntField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jfloat, field_type>)
			{
				if (not_valid) return jfloat(0.f);
				return get_env()->GetStaticFloatField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jlong, field_type>)
			{
				if (not_valid) return jlong(0LL);
				return get_env()->GetStaticLongField(owner_klass, id);
			}
			if constexpr (std::is_same_v<jdouble, field_type>)
			{
				if (not_valid) return jdouble(0.0);
				return get_env()->GetStaticDoubleField(owner_klass, id);
			}
		}

		operator field_type() const
		{
			return get();
		}

	private:
		inline static std::atomic<jfieldID> id{};
	};

	template<typename o_klass, typename field_type, string_litteral field_name>
	using static_field = multi_static_field < o_klass, field_type, string_litterals{ field_name } > ;



	template<typename o_klass, typename field_type, string_litterals field_names>
	class multi_field
	{
	public:

		static consteval auto get_names()
		{
			return field_names;
		}

		static consteval auto get_signatures()
		{
			return get_signatures_for_type<field_type>();
		}

		static consteval auto get_descriptive_name()
		{
			return concat< string_litterals_join(field_names), " : ", string_litterals_join(get_signatures()) >();
		}

		static void init_id()
		{
			if (jfieldID(id)) return;
			jclass owner_klass = get_cached_jclass<o_klass>();
			jfieldID new_id = nullptr;
			if (owner_klass)
			{
				tuple_litteral_foreach<field_names>([&new_id, owner_klass]<size_t i>(const auto& field_name)
				{
					if (new_id) return;
					constexpr string_litterals signatures = get_signatures();
					new_id = get_env()->GetFieldID(owner_klass, field_name, tuple_litteral_get_or_last<i>(signatures));
				});

				if (new_id) id = new_id;
			}
			assertm(new_id, (std::string_view)(concat<"failed to find fieldID: ", get_descriptive_name()>()));
		}

		operator jfieldID() const
		{
			init_id();
			return id;
		}

		multi_field(const empty_members& m) :
			m(m)
		{
		}

		multi_field(const multi_field& other) = delete; // make sure field won't be copied (we store a empty_members reference which must not be copied)
		multi_field(multi_field&& other) = delete;

		multi_field& operator=(const field_type& new_value)
		{
			set(new_value);
			return *this;
		}

		void set(const field_type& new_value)
		{
			init_id();
			assertm(m.get_jobject(), (std::string_view)(concat<"called set on a non static field with null object_instance : ", get_descriptive_name()>()));
			jclass owner_klass = get_cached_jclass<o_klass>();
			if (!jfieldID(id) || !owner_klass || !m.get_jobject()) return;

			if constexpr (!is_jni_primitive_type<field_type>)
				return get_env()->SetObjectField(m.get_jobject(), id, (jobject)new_value);

			if constexpr (std::is_same_v<jboolean, field_type>)
				return get_env()->SetBooleanField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jbyte, field_type>)
				return get_env()->SetByteField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jchar, field_type>)
				return get_env()->SetCharField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jshort, field_type>)
				return get_env()->SetShortField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jint, field_type>)
				return get_env()->SetIntField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jfloat, field_type>)
				return get_env()->SetFloatField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jlong, field_type>)
				return get_env()->SetLongField(m.get_jobject(), id, new_value);

			if constexpr (std::is_same_v<jdouble, field_type>)
				return get_env()->SetDoubleField(m.get_jobject(), id, new_value);
		}

		auto get() const
		{
			init_id();
			assertm(m.get_jobject(), (std::string_view)(concat<"called get on a non static field with null object_instance : ", get_descriptive_name()>()));
			jclass owner_klass = get_cached_jclass<o_klass>();
			bool not_valid = (!jfieldID(id) || !owner_klass || !m.get_jobject());
			if constexpr (!is_jni_primitive_type<field_type>)
			{
				if (not_valid) return field_type(nullptr);
				return field_type(get_env()->GetObjectField(m.get_jobject(), id));
			}
			if constexpr (std::is_same_v<jboolean, field_type>)
			{
				if (not_valid) return jboolean(JNI_FALSE);
				return get_env()->GetBooleanField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jbyte, field_type>)
			{
				if (not_valid) return jbyte(0);
				return get_env()->GetByteField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jchar, field_type>)
			{
				if (not_valid) return jchar(0);
				return get_env()->GetCharField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jshort, field_type>)
			{
				if (not_valid) return jshort(0);
				return get_env()->GetShortField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jint, field_type>)
			{
				if (not_valid) return jint(0);
				return get_env()->GetIntField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jfloat, field_type>)
			{
				if (not_valid) return jfloat(0.f);
				return get_env()->GetFloatField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jlong, field_type>)
			{
				if (not_valid) return jlong(0LL);
				return get_env()->GetLongField(m.get_jobject(), id);
			}
			if constexpr (std::is_same_v<jdouble, field_type>)
			{
				if (not_valid) return jdouble(0.0);
				return get_env()->GetDoubleField(m.get_jobject(), id);
			}
		}

		operator field_type() const
		{
			return get();
		}

	private:
		const empty_members& m;
		inline static std::atomic<jfieldID> id{};
	};

	template<typename o_klass, typename field_type, string_litteral field_name>
	using field = multi_field< o_klass, field_type, string_litterals{ field_name } > ;

	template<typename o_klass, typename method_return_type, string_litterals method_names, class... method_parameters_type>
	class multi_static_method
	{
	public:

		static consteval auto get_descriptive_name()
		{
			return string_litterals_join(method_names);
		}

		static consteval auto get_signatures()
		{
			return tuple_litteral_map<method_names>(
			[]<size_t i, auto method_name>()
			{
				return concat< "(", tuple_litteral_get_or_last<i>(get_signatures_for_type<method_parameters_type>())..., ")", tuple_litteral_get_or_last<i>(get_signatures_for_type<method_return_type>()) >();
			});
		}

		multi_static_method() = default;

		multi_static_method(const multi_static_method& other) = delete; // make sure method won't be copied (we store a empty_members reference which must not be copied)
		multi_static_method(multi_static_method&& other) = delete;

		static void init_id()
		{
			if (jmethodID(id)) return;
			jclass owner_klass = get_cached_jclass<o_klass>();
			jmethodID new_id = nullptr;
			if (owner_klass)
			{
				tuple_litteral_foreach<method_names>([&new_id, owner_klass]<size_t i>(const auto& field_name)
				{
					if (new_id) return;
					constexpr string_litterals signatures = get_signatures();
					new_id = get_env()->GetStaticMethodID(owner_klass, field_name, tuple_litteral_get_or_last<i>(signatures));
				});

				if (new_id) id = new_id;
			}
			assertm(new_id, (std::string_view)(concat<"failed to find methodID: ", get_descriptive_name()>()));
		}

		operator jmethodID() const
		{
			init_id();
			return id;
		}

		auto operator()(const method_parameters_type&... method_parameters) const
		{
			return call(method_parameters...);
		}

		auto call(const method_parameters_type&... method_parameters) const
		{
			init_id();
			jclass owner_klass = get_cached_jclass<o_klass>();
			bool not_valid = !jmethodID(id) || !owner_klass;
			if constexpr (std::is_void_v<method_return_type>)
			{
				if (not_valid) return;
				get_env()->CallStaticVoidMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				return;
			}

			if constexpr (!is_jni_primitive_type<method_return_type> && !std::is_void_v<method_return_type>)
			{
				if (not_valid) return method_return_type(nullptr);
				return method_return_type(get_env()->CallStaticObjectMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...));
			}
			if constexpr (std::is_same_v<jboolean, method_return_type>)
			{
				if (not_valid) return jboolean(JNI_FALSE);
				return get_env()->CallStaticBooleanMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jbyte, method_return_type>)
			{
				if (not_valid) return jbyte(0);
				return get_env()->CallStaticByteMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jchar, method_return_type>)
			{
				if (not_valid) return jchar(0);
				return get_env()->CallStaticCharMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jshort, method_return_type>)
			{
				if (not_valid) return jshort(0);
				return get_env()->CallStaticShortMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jint, method_return_type>)
			{
				if (not_valid) return jint(0);
				return get_env()->CallStaticIntMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jfloat, method_return_type>)
			{
				if (not_valid) return jfloat(0.f);
				return get_env()->CallStaticFloatMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jlong, method_return_type>)
			{
				if (not_valid) return jlong(0LL);
				return get_env()->CallStaticLongMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jdouble, method_return_type>)
			{
				if (not_valid) return jdouble(0.0);
				return get_env()->CallStaticDoubleMethod(owner_klass, id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
		}

	private:
		inline static std::atomic<jmethodID> id{};
	};

	template<typename o_klass, typename method_return_type, string_litteral method_name, class... method_parameters_type>
	using static_method = multi_static_method < o_klass, method_return_type, string_litterals{ method_name }, method_parameters_type... > ;


	template<typename o_klass, typename method_return_type, string_litterals method_names, class... method_parameters_type>
	class multi_method
	{
	public:
		static consteval auto get_descriptive_name()
		{
			return string_litterals_join(method_names);
		}

		static consteval auto get_signatures()
		{
			return tuple_litteral_map<method_names>(
				[]<size_t i, auto method_name>()
			{
				return concat< "(", tuple_litteral_get<i>(get_signatures_for_type<method_parameters_type>())..., ")", tuple_litteral_get<i>(get_signatures_for_type<method_return_type>()) >();
			});
		}

		static void init_id()
		{
			if (jmethodID(id)) return;
			jclass owner_klass = get_cached_jclass<o_klass>();
			jmethodID new_id = nullptr;
			if (owner_klass)
			{
				tuple_litteral_foreach<method_names>([&new_id, owner_klass]<size_t i>(const auto& field_name)
				{
					if (new_id) return;
					constexpr string_litterals signatures = get_signatures();
					new_id = get_env()->GetMethodID(owner_klass, field_name, tuple_litteral_get_or_last<i>(signatures));
				});

				if (new_id) id = new_id;
			}
			assertm(new_id, (std::string_view)(concat<"failed to find methodID: ", get_descriptive_name()>()));
		}

		operator jmethodID() const
		{
			init_id();
			return id;
		}

		multi_method(const empty_members& m) :
			m(m)
		{
		}

		multi_method(const multi_method& other) = delete; // make sure method won't be copied (we store a empty_members reference which must not be copied)
		multi_method(multi_method&& other) = delete;

		auto operator()(const method_parameters_type&... method_parameters) const
		{
			return call(method_parameters...);
		}

		auto call(const method_parameters_type&... method_parameters) const
		{
			init_id();
			assertm(m.get_jobject(), (std::string_view)(concat<"called call on a non static method with null object_instance : ", get_descriptive_name()>()));
			jclass owner_klass = get_cached_jclass<o_klass>();
			bool not_valid = !jmethodID(id) || !owner_klass || !m.get_jobject();
			if constexpr (std::is_void_v<method_return_type>)
			{
				if (not_valid) return;
				get_env()->CallVoidMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
				return;
			}

			if constexpr (!is_jni_primitive_type<method_return_type> && !std::is_void_v<method_return_type>)
			{
				if (not_valid) return method_return_type(nullptr);
				return method_return_type(get_env()->CallObjectMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...));
			}
			if constexpr (std::is_same_v<jboolean, method_return_type>)
			{
				if (not_valid) return jboolean(JNI_FALSE);
				return get_env()->CallBooleanMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jbyte, method_return_type>)
			{
				if (not_valid) return jbyte(0);
				return get_env()->CallByteMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jchar, method_return_type>)
			{
				if (not_valid) return jchar(0);
				return get_env()->CallCharMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jshort, method_return_type>)
			{
				if (not_valid) return jshort(0);
				return get_env()->CallShortMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jint, method_return_type>)
			{
				if (not_valid) return jint(0);
				return get_env()->CallIntMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jfloat, method_return_type>)
			{
				if (not_valid) return jfloat(0.f);
				return get_env()->CallFloatMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jlong, method_return_type>)
			{
				if (not_valid) return jlong(0LL);
				return get_env()->CallLongMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
			if constexpr (std::is_same_v<jdouble, method_return_type>)
			{
				if (not_valid) return jdouble(0.0);
				return get_env()->CallDoubleMethod(m.get_jobject(), id, std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...);
			}
		}

	private:
		const empty_members& m;
		inline static std::atomic<jmethodID> id;
	};

	template<typename o_klass, typename method_return_type, string_litteral method_name, class... method_parameters_type>
	using method = multi_method < o_klass, method_return_type, string_litterals{ method_name }, method_parameters_type... > ;


	template<typename o_klass, class... method_parameters_type>
	class constructor : public multi_method< o_klass, void, string_litterals{ "<init>" }, method_parameters_type... >
	{
	public:
		using multi_method < o_klass, void, string_litterals{ "<init>" }, method_parameters_type... > ::multi_method;

		o_klass new_object(const method_parameters_type&... method_parameters)
		{
			return o_klass{ jni::get_env()->NewObject(get_cached_jclass<o_klass>(), jmethodID(*this), std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...) };
		}
	};


	template<string_litterals class_names, class members_type>
	class multi_klass : public members_type
	{
	public:

		explicit multi_klass(reference_type ref_type = reference_type::LOCAL) : multi_klass(jni::object_wrapper{ ref_type }) {}
		explicit multi_klass(const object_wrapper& other) : members_type(other) {} // very important to not copy jni::field and method
		explicit multi_klass(object_wrapper&& other) : members_type(std::move(other)) {}

		multi_klass(const multi_klass& other) : multi_klass((const object_wrapper&)other) {}
		multi_klass(multi_klass&& other) noexcept : multi_klass(static_cast<object_wrapper&&>(other)) {}

		multi_klass new_global_ref() const
		{
			return multi_klass({ this->get_jobject(), reference_type::GLOBAL });
		}

		multi_klass& operator=(const multi_klass& other)
		{
			object_wrapper::operator=(other);
			return *this;
		}

		multi_klass& operator=(multi_klass&& other) noexcept
		{
			object_wrapper::operator=(std::move(other));
			return *this;
		}

		template<class... method_parameters_type>
		static multi_klass new_object(jni::constructor<multi_klass, method_parameters_type...> members_type::* constructor, const method_parameters_type&... method_parameters) // tbh I was just playing with member pointers
		{
			multi_klass tmp{}; //lmao
			return multi_klass{ jni::get_env()->NewObject(get_cached_jclass<multi_klass>(), jmethodID(tmp.*constructor), std::conditional_t<is_jni_primitive_type<method_parameters_type>, method_parameters_type, jobject>(method_parameters)...) };
		}

		static consteval auto get_descriptive_name()
		{
			return string_litterals_join(class_names);
		}

		static consteval auto get_names()
		{
			return class_names;
		}

		static consteval auto get_signatures()
		{
			return tuple_litteral_map<class_names>(
			[]<size_t i, auto class_name>()
			{
				return concat<"L", class_name, ";">(); 
			});
		}
	};

	template<string_litteral class_name, class members_type>
	using klass = multi_klass<string_litterals{class_name}, members_type>;

	class frame
	{
	public:
		frame(jint capacity = 16)
		{
			get_env()->PushLocalFrame(capacity);
		}
		~frame()
		{
			get_env()->PopLocalFrame(nullptr);
		}
	};
}