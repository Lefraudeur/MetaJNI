# Mapping Generator For MetaJNI2

This program parses a mapping file in [tiny V2](https://wiki.fabricmc.net/documentation:tiny2) format by fabric MC  
and converts it into a mappings_auto.hpp header for [metaJNI2](https://github.com/Lefraudeur/MetaJNI/tree/metaJNI2).

It was used in https://github.com/Lefraudeur/MetaJNI/tree/example-base-forge-1.7.10/ (metaJNI1 format but it's almost the same),  

A test input file and its output is available in the [test](https://github.com/Lefraudeur/MetaJNI/tree/metaJNI2-mapping-generator/test) folder.

## Usage
A prebuilt executable is available in the releases.
```
metaJNI2-gen <tiny_mapping_file> <obfuscated_namespace> <unobfuscated_namespace>
```
tiny_mapping_file must be in ASCII (multibyte utf8 characters aren't supported) LF (\n line ending not windows \r\n)  
Example with a tiny V2 file named `mappings.tiny`:
```
tiny	2	0	obfuscated	searge	named
c	a	net/minecraft/util/EnumChatFormatting	net/minecraft/util/EnumChatFormatting
	f	Z	A	field_96303_A	fancyStyling
```
At line 1 you can see the names of the different namespaces : `obfuscated`, `searge`, `named`.  
At line 2 you can deduce that the field's `obfuscated` name (as in the vanilla minecraft jar) is `A`, its `searge` name (used in forge) is `field_96303_A` and its `named` name is `fancyStyling`.  
`Z` means the field is of type `boolean`.

Thus with the command `metaJNI2-gen mappings.tiny obfuscated named`, the following file will be generated :
```C++
#pragma once
#include <meta_jni.hpp>

namespace net::minecraft::util
{
	KLASS_DECLARATION(EnumChatFormatting, "a");
}
namespace net::minecraft::util
{
	BEGIN_KLASS_MEMBERS(EnumChatFormatting)
		field<jboolean, "A"> fancyStyling{ *this };
	END_KLASS_MEMBERS()
}
```

Some characters / names are allowed in java but not in c++, as such the string `_$` will be appended to illegal names and illegal characters will be replaced by `_$`.  
The list of illegal characters / names may not be complete. It's located in [src/mapping_parser/klass/klass.hpp#L211](https://github.com/Lefraudeur/MetaJNI/tree/metaJNI2-mapping-generator/src/mapping_parser/klass/klass.hpp#L211)

## Issues
Most of the time, the program will output a file **too big** for the compiler, or with some errors.  
So the generated file is not really usable as is, but you can still copy only the parts you need to make it smaller.  
That's an issue I didn't think of when designing metaJNI.

⚠️ **I made this a long time ago for myself and did not intend to publish it at first. So the code is messy and there is almost no error handling.  
If the parameters or the mapping file are incorrect, the program will either crash or behave unpredictably and output garbage.**

## Static fields and methods

The tiny V2 format does not give information about whether a field/method is static or not.  
In consequence, it will be assumed that all fields/methods are non static by default when generating the metaJNI2 mappings.  
However the tiny V2 format allows for comments by starting a line with `c`.  
This is why I made so that the generator is able to detect if a field/method has a comment saying exactly `static`, and write the appropriate metaJNI2 code.  
For example if the generator sees in the tiny V2 file :
```
	f	[La;	D	$VALUES	$VALUES
		c	static
```
It will deduce that field named D/$VALUES is static and use the appropriate metaJNI2 type `static_field`.

I made a small java program that is able to add these `static` indicators automatically given the mapping file and the client / server jar file containing the fields/methods referenced by the mapping file.  
It's in [extra_tools/tiny_static_info_adder](https://github.com/Lefraudeur/MetaJNI/tree/metaJNI2-mapping-generator/extra_tools/tiny_static_info_adder/)

## How to get tiny V2 mapping files ?

Here are different sources from which you can get mappings :

MCP:  
mcp version list: https://maven.minecraftforge.net/de/oceanlabs/mcp/versions.json  
mcp format: https://maven.minecraftforge.net/de/oceanlabs/mcp/{channel}/{mcp-version}-{mc-version}/{channel}-{mcp-version}-{mc-version}.zip  
ex: https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp_stable/39-1.12/mcp_stable-39-1.12.zip

SRG  
srg format https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp/{mc-version}/mcp-{mc-version}-srg.zip  
tsrg format https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp_config/{mc-version}/mcp_config-{mc-version}.zip  (1.13+)  
ex: https://maven.minecraftforge.net/de/oceanlabs/mcp/mcp/1.12.2/mcp-1.12.2-srg.zip

YARN:  
https://maven.fabricmc.net/net/fabricmc/yarn/

OFFICIAL:  
https://piston-meta.mojang.com/mc/game/version_manifest_v2.json

In case your mapping file is not in tiny V2 format, there is a good chance you can use https://github.com/FabricMC/mapping-io to convert it to tiny V2.  
Using this tool, you can also merge multiple mapping files together.

## Things that may be useful to do
- Cleanup the code, better error handling
- Support for multi byte utf-8 characters
- Clarify how to get tiny V2 mapping files
- Being able to generate a metaJNI2 header from a jar file
- Being able to generate only the metaJNI2 code for a specific class and its dependencies to reduce the header size
- Separe the output in multiple files, one file per class