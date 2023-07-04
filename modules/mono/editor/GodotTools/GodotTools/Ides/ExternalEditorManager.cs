using Godot;

#nullable enable

namespace GodotTools.Ides
{
    public abstract class ExternalEditorManager
    {
        public IdeManager IdeManager => GodotSharpEditor.Instance.IdeManager;

        public abstract Error Launch(Script script, int line, int column);
    }
}
