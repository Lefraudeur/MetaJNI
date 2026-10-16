package io.github.lefraudeur;

import io.github.lefraudeur.parser.JarParser;
import net.fabricmc.mappingio.MappedElementKind;
import net.fabricmc.mappingio.MappingReader;
import net.fabricmc.mappingio.MappingWriter;
import net.fabricmc.mappingio.adapter.ForwardingMappingVisitor;
import net.fabricmc.mappingio.format.MappingFormat;
import net.fabricmc.mappingio.format.tiny.Tiny2FileWriter;
import org.jetbrains.annotations.Nullable;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Main {
    public static void main(String[] args) throws IOException {

        if (args.length < 2)
        {
            System.err.println("Usage: " + "java -jar tiny-static-info-adder.jar <tiny_mapping_file> <client_jar> [server_jar]");
            return;
        }

        String tiny_file = args[0];
        String client_file = args[1];
        String server_file = null;
        if (args.length >= 3)
            server_file = args[2];

        final JarParser joined = parseJar(client_file, server_file);

        MappingReader.read(Paths.get(tiny_file), MappingFormat.TINY_2_FILE,
                new ForwardingMappingVisitor(MappingWriter.create(Paths.get("output.tiny"), MappingFormat.TINY_2_FILE))
                {
                    private String currentClass = null;
                    private String currentName = null;
                    private String currentDescriptor = null;

                    @Override
                    public boolean visitClass(String srcName) throws IOException
                    {
                        currentClass = srcName;
                        return super.visitClass(srcName);
                    }

                    @Override
                    public boolean visitField(String srcName, @Nullable String srcDesc) throws IOException
                    {
                        currentName = srcName;
                        currentDescriptor = srcDesc;
                        return super.visitField(srcName, srcDesc);
                    }

                    @Override
                    public boolean visitMethod(String srcName, @Nullable String srcDesc) throws IOException
                    {
                        currentName = srcName;
                        currentDescriptor = srcDesc;
                        return super.visitMethod(srcName, srcDesc);
                    }

                    @Override
                    public boolean visitElementContent(MappedElementKind targetKind) throws IOException
                    {
                        super.visitElementContent(targetKind);

                        if (targetKind == MappedElementKind.FIELD || targetKind == MappedElementKind.METHOD)
                        {
                            if (joined.isFieldStatic(currentClass, currentName, currentDescriptor))
                                visitComment(targetKind, "static");
                        }
                        return true;
                    }
                });
    }

    private static JarParser parseJar(String client_file, String server_file) throws IOException {
        JarParser joined = new JarParser(Paths.get( client_file));
        if (server_file != null)
        {
            JarParser server = new JarParser(Paths.get(server_file));
            joined = JarParser.combine(server, joined);
        }

        return joined;
    }
}