using System;
using System.Collections.Generic;
using Godot;
using OS = GodotTools.Utils.OS;

#nullable enable

namespace GodotTools.Ides.CustomEditor
{
    public class CustomEditorManager : ExternalEditorManager
    {
        public override Error Launch(Script script, int line, int column)
        {
            var editorSettings = EditorInterface.Singleton.GetEditorSettings();

            string file = ProjectSettings.GlobalizePath(script.ResourcePath);
            string project = ProjectSettings.GlobalizePath("res://");
            // Since ProjectSettings.GlobalizePath replaces only "res:/", leaving a trailing slash, it is removed here.
            project = project[..^1];
            string execCommand = editorSettings.GetSetting(GodotSharpEditor.Settings.CustomExecPath).As<string>();
            string execArgs = editorSettings.GetSetting(GodotSharpEditor.Settings.CustomExecPathArgs).As<string>();
            var args = new List<string>();
            int from = 0;
            int numChars = 0;
            bool insideQuotes = false;
            bool hasFileFlag = false;

            execArgs = execArgs
                .Replace("{line}", line.ToString(), StringComparison.OrdinalIgnoreCase)
                .Replace("{col}", column.ToString(), StringComparison.OrdinalIgnoreCase)
                .Trim()
                .Replace("\\\\", "\\", StringComparison.OrdinalIgnoreCase);

            for (int i = 0; i < execArgs.Length; ++i)
            {
                if (execArgs[i] == '"' && (i == 0 || execArgs[i - 1] != '\\') && i != execArgs.Length - 1)
                {
                    if (!insideQuotes)
                    {
                        from++;
                    }
                    insideQuotes = !insideQuotes;
                }
                else if ((execArgs[i] == ' ' && !insideQuotes) || i == execArgs.Length - 1)
                {
                    if (i == execArgs.Length - 1 && !insideQuotes)
                    {
                        numChars++;
                    }

                    var arg = execArgs.Substr(from, numChars);
                    if (arg.Contains("{file}", StringComparison.OrdinalIgnoreCase))
                    {
                        hasFileFlag = true;
                    }

                    arg = arg.Replace("{project}", project, StringComparison.OrdinalIgnoreCase);
                    arg = arg.Replace("{file}", file, StringComparison.OrdinalIgnoreCase);
                    args.Add(arg);

                    from = i + 1;
                    numChars = 0;
                }
                else
                {
                    numChars++;
                }
            }

            if (!hasFileFlag)
            {
                args.Add(file);
            }

            OS.RunProcess(execCommand, args);
            return Error.Ok;
        }
    }
}
