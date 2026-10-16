package io.github.lefraudeur.parser;

class ParsedFieldMethod
{
    public final String name;
    public final String descriptor;
    public final boolean isStatic;

    public ParsedFieldMethod(String name, String descriptor, boolean isStatic)
    {
        this.name = name;
        this.descriptor = descriptor;
        this.isStatic = isStatic;
    }

    @Override
    public boolean equals(Object obj)
    {
        if (!(obj instanceof ParsedFieldMethod other))
            return super.equals(obj);
        return descriptor.equals(other.descriptor) && name.equals(other.name);
    }
}
