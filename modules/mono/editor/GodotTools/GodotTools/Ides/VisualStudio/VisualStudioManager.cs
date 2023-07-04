using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Godot;
using GodotTools.Internals;
using OS = GodotTools.Utils.OS;

#nullable enable

namespace GodotTools.Ides.VisualStudio
{
    public class VisualStudioManager : ExternalEditorManager
    {
        public override Error Launch(Script script, int line, int column)
        {
            if (!OperatingSystem.IsWindows())
            {
                GD.PushError("Visual Studio not supported on this platform.");
                return Error.Failed;
            }

            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);

            string command = Path.Combine(GodotSharpDirs.DataEditorToolsDir, "GodotTools.OpenVisualStudio.exe");

            var args = new List<string>
            {
                GodotSharpDirs.ProjectSlnPath,
                line >= 0 ? $"{scriptPath};{line + 1};{column + 1}" : scriptPath
            };

            OS.RunProcess(command, args);

            return Error.Ok;
        }
    }
}
