# Tinny Static Info Adder

Adds `static` indicators automatically to a tiny V2 mapping file, given the client / server jar file containing the fields/methods referenced by the mapping file.  
Example output :
```
	f	[La;	D	$VALUES	$VALUES
		c	static
```

## Usage
Requires Java 17, prebuilt jar available in releases.
```
java -jar tiny-static-info-adder.jar <tiny_mapping_file> <client_jar> [server_jar]
```
`client_jar` must be a jar file that contains fields/methods referenced by `tiny_mapping_file` usually the minecraft client downloaded from https://piston-meta.mojang.com/mc/game/version_manifest_v2.json  
In case `tiny_mapping_file` refers to fields/methods that aren't in `client_jar` but are in a second jar file (for example in the minecraft server jar), specify the second jar with the optional `server_jar` parameter.

The fields / methods in `client_jar` must be using the source names of `tiny_mapping_file` (names from its first namespace)