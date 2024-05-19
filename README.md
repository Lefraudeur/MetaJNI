# MetaJNI
A header only JNI wrapper that makes using jni safer and easier while having almost no performance impact.
This branch uses cmake for the example and supports both Windows and Linux (you will have to install X11 dev library).\
An older version that uses visual studio 2022 and works only on windows is available here:\
https://github.com/Lefraudeur/MetaJNI/tree/old-vs22-windows

## Advantages over raw JNI:
- [Syntax](#showcase) as close as possible to direct java code
- Signature deduction at compile time, so you don't have to write "(ILjava/lang/String;[I)J" anymore
- [Mapping](#create-mappings) system allows for tab completion and makes interacting with obfuscated java code easier
- Errors at compile time, type safety, minimize developer errors
- Multithreading and global reference management

### With MetaJNI, your code looks this simple :<a id="showcase"></a>
```C++
maps::Minecraft Minecraft{};

//get a static field
maps::Minecraft theMinecraft = Minecraft.theMinecraft.get();

//get a non static field
jint displayWidth = theMinecraft.displayWidth.get();

//set a non static field
theMinecraft.displayWidth = 100;

//call a method with parameters
theMinecraft.resize(100, 100);

//nice chaining
std::string thePlayer_clientBrand = Minecraft.theMinecraft.get().thePlayer.get().getClientBrand().to_string();

//And more...
```
See: [main.cpp](https://github.com/Lefraudeur/MetaJNI/blob/master/main.cpp)

## Get Started
The only file you have to include in your project is [meta_jni.hpp](https://github.com/Lefraudeur/MetaJNI/blob/master/meta_jni.hpp)
Other files are part of the exemple project, it's a dll injectable into minecraft vanilla 1.8.9 to showcase and test all features,\
but the **library does not depend on minecraft** and can be used anywhere you would use normal jni.
### Setup JNIEnv* :
The library does not provide any method to get the JNIEnv*,\
So **for each thread** that uses the library, you have to call
```C++
jni::set_thread_env(env);
```
### Create mappings :
See: [mappings.hpp](https://github.com/Lefraudeur/MetaJNI/blob/master/mappings.hpp)
Start by creating a header file like `mappings.hpp`, it's also recommended to put further definitions in a namespace like `maps::`
- #### Define a class
	```C++
	BEGIN_KLASS_DEF(ClassName, "RealJavaClassName")
	//jni::field and jni::method definitions here
	END_KLASS_DEF()
	```
	Here `ClassName` designates the class name in the C++ side, it's the jni::klass<> type you can access via maps::ClassName.
	`"RealJavaClassName"` designates the class name in the java side, which can be obfuscated.\
	It is also possible to define a class that inherits its methods and fields from another class with `BEGIN_KLASS_DEF_EX`
	
	```C++
	BEGIN_KLASS_DEF_EX(ClassName, "RealJavaClassName", ParentPreviousDefinedClass)
	//jni::field and jni::method definitions here
	END_KLASS_DEF()
	```
	Where `ParentPreviousDefinedClass` is the parent class previously defined by `BEGIN_KLASS_DEF(_EX)`
- #### Define a field
	```C++
	jni::field<FieldType, "realJavaFieldName", is_static> fieldName{ *this };
	```
	FieldType represents the type of the field, which can be any of :\
	`jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble, jni::array<element_type>, jni::klass<> (defined by BEGIN_KLASS_DEF)`.

	"realJavaFieldName" must be the name of the field in the java side, possibly obfuscated.

	is_static can be `jni::STATIC` or `jni::NOT_STATIC`, if not specified it defaults to `jni::NOT_STATIC`.
- #### Define a method
	```C++
	jni::method<MethodReturnType, "realJavaMethodName", is_static, parameterType1, parameterType2, parameterTypeN...> methodName{ *this };
	```
	MethodReturnType represents the return type of the method, which can be any of :\
	`void, jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble, jni::array<element_type>, jni::klass<> (defined by BEGIN_KLASS_DEF)`.

	"realJavaMethodName" must be the name of the method in the java side, possibly obfuscated.

	is_static can be `jni::STATIC` or `jni::NOT_STATIC`.

	Remaining parameters are the types of the method's parameters, in their corresponding java order, which can be any of :\
	`jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble, jni::array<element_type>, jni::klass<> (defined by BEGIN_KLASS_DEF)`.
### Cleanup library :
**Once your program exits**, or when you don't want to use the library anymore, don't forget to call
```C++
jni::shutdown();
```

### Useful Info :

#### Array support
Places that accept a `jni::klass<>` usually also accept a `jni::array<element_type>`, where element_type can be any of:\
`jboolean, jbyte, jchar, jshort, jint, jfloat, jlong, jdouble, jni::array<element_type>, jni::klass<> (defined by BEGIN_KLASS_DEF)`

To iterate over an array easily, you can convert it to an std::vector with `.to_vector()`.

You can also allocate a new array with `jni::array<element_type>::create({elements...})`.

#### Reference management
MetaJNI does not provide any method to manage local references, they are still managed as usual :\
they follow the lifetime of a JNI frame which can be pushed and popped with\
`env->PushLocalFrame(local_ref_count);` and `env->PopLocalFrame(nullptr);`.

If you need a reference to live across JNI frames or threads, MetaJNI provides an easy way to create global references that will be destroyed once the corresponding C++ object is destroyed :\
Simply pass `true` to the `jni::klass` or `jni::array` constructor. For example:
```C++
maps::Minecraft global_theMinecraft = maps::Minecraft(local_theMinecraft, true);
```

#### Object creation
- #### Define a constructor
	A `jni::constructor<parameterType1, parameterType2, parameterTypeN...>` is basically the same as `jni::method<void, "<init>", jni::NOT_STATIC, parameterType1, parameterType2, parameterTypeN...>`\
	add it to your klass definition in the mappings, for example :
	```
	BEGIN_KLASS_DEF(URL, "java/net/URL")
		jni::constructor<String> constructor{ *this }; // the java/net/URL constructor takes a String as parameter
	END_KLASS_DEF()
	```
- #### Construct a new object
	New objects can be created using `CLASS_NAME_::new_object(&CLASS_NAME_::constructor, parameters...)` \
	Where CLASS_NAME is a jni::klass<> type defined by BEGIN_KLASS_DEF, for example :
	```
	maps::URL url = maps::URL::new_object(&maps::URL::constructor, String.create("http://www.example.com/docs/resource1.html"));
	```

#### Warning / Remarks / Downsides...
The way you create new objects or call static methods can be a bit confusing :\
when you construct a jni::klass, no java object is created on the jvm, it is simply a wrapper for an existing jobject,\
if you don't give any jobject to the jni::klass constructor, then it defaults to nullptr so you can only call static java methods.\
`maps::Minecraft Minecraft{};` \
This line doesn't create a Minecraft object on the jvm. \
It is not really natural, but  I can't think of a better way... \
I try to find the best compromise between the easiness of the mappings and of the actual code

While c++ templates are fun, useful, and very powerful, \
coding another program that writes repetitive code for you would give way more possibilites