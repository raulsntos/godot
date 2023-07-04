using System;
using System.Collections.Generic;
using System.IO;
using Godot;
using GodotTools.Internals;
using GodotTools.Utils;
using OS = GodotTools.Utils.OS;
using File = System.IO.File;

#nullable enable

namespace GodotTools.Ides.VisualStudioCode
{
    public class VisualStudioCodeManager : ExternalEditorManager
    {
        // The package path is '/Applications/Visual Studio Code.app'
        private const string VSCodeBundleId = "com.microsoft.VSCode";

        private static string? _vsCodePath;

        private static readonly string[] VSCodeNames =
        {
            "code", "code-oss", "vscode", "vscode-oss", "visual-studio-code", "visual-studio-code-oss"
        };

        public override Error Launch(Script script, int line, int column)
        {
            string command;

            var args = new List<string>();

            if (OperatingSystem.IsMacOS() && Internal.IsMacOSAppBundleInstalled(VSCodeBundleId))
            {
                command = "/usr/bin/open";

                args.Add("-b");
                args.Add(VSCodeBundleId);

                // The reusing of existing windows made by the 'open' command might not choose a window that is
                // editing our folder. It's better to ask for a new window and let VSCode do the window management.
                args.Add("-n");

                // The open process must wait until the application finishes (which is instant in VSCode's case)
                args.Add("--wait-apps");

                args.Add("--args");
            }
            else
            {
                if (string.IsNullOrEmpty(_vsCodePath) || !File.Exists(_vsCodePath))
                {
                    // Try to search it again if it wasn't found last time or if it was removed from its location.
                    _vsCodePath = VSCodeNames.SelectFirstNotNull(OS.PathWhich);
                }

                if (string.IsNullOrEmpty(_vsCodePath))
                {
                    GD.PushError("Cannot find code editor: VSCode");
                    return Error.FileNotFound;
                }

                command = _vsCodePath;
            }

            args.Add(Path.GetDirectoryName(GodotSharpDirs.ProjectSlnPath)!);

            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);

            if (line >= 0)
            {
                args.Add("-g");
                args.Add($"{scriptPath}:{line + 1}:{column + 1}");
            }
            else
            {
                args.Add(scriptPath);
            }

            OS.RunProcess(command, args);

            return Error.Ok;
        }
    }
}
