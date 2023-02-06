#pragma warning disable CS0169

namespace Godot.SourceGenerators.Sample
{
    public partial class ScriptBoilerplate : Node
    {
        private NodePath _nodePath;
        private int _velocity;

#if REAL_T_IS_DOUBLE
        public override void _Process(double delta)
#else
        public override void _Process(float delta)
#endif
        {
            _ = delta;

            base._Process(delta);
        }

        public int Bazz(StringName name)
        {
            _ = name;
            return 1;
        }

        public void IgnoreThisMethodWithByRefParams(ref int a)
        {
            _ = a;
        }
    }

    partial struct OuterClass
    {
        public partial class NesterClass : RefCounted
        {
            public override Variant _Get(StringName property) => default;
        }
    }
}
