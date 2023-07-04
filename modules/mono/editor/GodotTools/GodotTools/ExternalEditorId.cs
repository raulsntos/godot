namespace GodotTools
{
    public enum ExternalEditorId : long
    {
        None,
        VisualStudio, // Windows-only.
        VisualStudioForMac, // MacOS-only.
        MonoDevelop,
        VSCode,
        Rider,
        CustomEditor,
    }
}
