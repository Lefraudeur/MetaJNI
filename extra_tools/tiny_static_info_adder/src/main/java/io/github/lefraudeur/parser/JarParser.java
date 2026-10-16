package io.github.lefraudeur.parser;

import org.objectweb.asm.*;

import java.io.IOException;
import java.nio.file.Path;
import java.util.*;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.logging.Logger;

public class JarParser
{
    private final Map<String, List<ParsedFieldMethod>> classFieldMap;

    public JarParser(JarParser other)
    {
        this.classFieldMap = new HashMap<>(other.classFieldMap);
    }

    public JarParser(Path jarPath) throws IOException
    {
        classFieldMap = new HashMap<>();
        JarFile jar = new JarFile(jarPath.toFile());
        Enumeration<JarEntry> entries = jar.entries();
        for (JarEntry entry = entries.nextElement(); entries.hasMoreElements(); entry = entries.nextElement())
        {
            if (entry.isDirectory()) continue;
            if (!entry.getName().endsWith(".class")) continue;
            ClassReader cr = new ClassReader(jar.getInputStream(entry));
            ClassVisitor cv = new ClassVisitor(Opcodes.ASM9)
            {
                private final List<ParsedFieldMethod> fields = new ArrayList<>();
                @Override
                public void visit(int version, int access, String name, String signature, String superName, String[] interfaces)
                {
                    classFieldMap.put(name, fields);
                    super.visit(version, access, name, signature, superName, interfaces);
                }

                @Override
                public FieldVisitor visitField(int access, String name, String descriptor, String signature, Object value)
                {
                    fields.add(new ParsedFieldMethod(name, descriptor, (access & Opcodes.ACC_STATIC) != 0));
                    return super.visitField(access, name, descriptor, signature, value);
                }

                @Override
                public MethodVisitor visitMethod(int access, String name, String descriptor, String signature, String[] exceptions)
                {
                    fields.add(new ParsedFieldMethod(name, descriptor, (access & Opcodes.ACC_STATIC) != 0));
                    return super.visitMethod(access, name, descriptor, signature, exceptions);
                }
            };
            cr.accept(cv, 0);
        }
    }

    public String getFieldDescriptor(String className, String fieldName)
    {
        List<ParsedFieldMethod> classFields = classFieldMap.get(className);
        if (classFields == null)
        {
            Logger.getGlobal().warning("No field info for class: " + className);
            return "Ljava/lang/Object;";
        }
        ParsedFieldMethod[] found = classFields.stream().
                filter((ParsedFieldMethod f) -> f.name.equals(fieldName)).
                toArray(ParsedFieldMethod[]::new);
        if (found.length == 0)
        {
            Logger.getGlobal().warning("Could not find descriptor in jar for field: " + className + "." + fieldName);
            return "Ljava/lang/Object;";
        }
        if (found.length > 1)
        {
            StringBuilder warning = new StringBuilder("Multiple descriptors found for: " + className + "." + fieldName + ":\n");
            for (ParsedFieldMethod f : found)
                warning.append("-").append(f.descriptor).append("\n");
            Logger.getGlobal().warning(warning + "\n\n");

        }
        return found[0].descriptor;
    }

    public boolean isFieldStatic(String className, String fieldName, String descriptor)
    {
        List<ParsedFieldMethod> fields = classFieldMap.get(className);
        if (fields == null) return false;

        for (ParsedFieldMethod f : fields)
            if (f.name.equals(fieldName) && f.descriptor.equals(descriptor))
                return f.isStatic;

        return false;
    }

    public static JarParser combine(JarParser parser1, JarParser parser2)
    {
        JarParser combined = new JarParser(parser1);
        for (Map.Entry<String, List<ParsedFieldMethod>> e : parser2.classFieldMap.entrySet())
        {
            List<ParsedFieldMethod> existingList = combined.classFieldMap.get(e.getKey());
            if (existingList != null)
            {
                for (ParsedFieldMethod f : e.getValue())
                    if (!existingList.contains(f))
                        existingList.add(f);
            }
            else
                combined.classFieldMap.put(e.getKey(), e.getValue());
        }
        return combined;
    }
}
