using System;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using Godot;
using GodotTools.Internals;
using JetBrains.Rider.PathLocator;

#nullable enable

namespace GodotTools.Ides.Rider
{
    public static class RiderPathManager
    {
        private const string EditorPathSettingName = "dotnet/editor/editor_path_optional";

        // Lazy-load JetBrains.Rider.PathLocator classes.
        private static UtilsData? _utils;
        private static UtilsData Utils => _utils ??= new();

        private class UtilsData
        {
            public RiderPathLocator Locator { get; }
            public RiderFileOpener Opener { get; }

            public UtilsData()
            {
                var riderLocatorEnvironment = new RiderLocatorEnvironment();
                Locator = new RiderPathLocator(riderLocatorEnvironment);
                Opener = new RiderFileOpener(riderLocatorEnvironment);
            }
        }

        private static string? GetRiderPathFromSettings()
        {
            var editorSettings = EditorInterface.Singleton.GetEditorSettings();
            if (editorSettings.HasSetting(EditorPathSettingName))
                return (string)editorSettings.GetSetting(EditorPathSettingName);
            return null;
        }

        public static void Initialize()
        {
            var editorSettings = EditorInterface.Singleton.GetEditorSettings();
            var editor = editorSettings.GetSetting(GodotSharpEditor.Settings.ExternalEditor).As<ExternalEditorId>();
            if (editor == ExternalEditorId.Rider)
            {
                if (!editorSettings.HasSetting(EditorPathSettingName))
                {
                    Globals.EditorDef(EditorPathSettingName, "");
                    editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
                    {
                        ["type"] = (int)Variant.Type.String,
                        ["name"] = EditorPathSettingName,
                        ["hint"] = (int)PropertyHint.File,
                        ["hint_string"] = ""
                    });
                }

                var riderPath = (string)editorSettings.GetSetting(EditorPathSettingName);
                if (File.Exists(riderPath))
                {
                    Globals.EditorDef(EditorPathSettingName, riderPath);
                    return;
                }

                var paths = Utils.Locator.GetAllRiderPaths();
                if (paths.Length == 0)
                    return;

                string newPath = paths.Last().Path;
                Globals.EditorDef(EditorPathSettingName, newPath);
                editorSettings.SetSetting(EditorPathSettingName, newPath);
            }
        }

        public static bool IsRider(string path)
        {
            if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
                return false;

            var fileInfo = new FileInfo(path);
            return fileInfo.Name.StartsWith("rider", StringComparison.OrdinalIgnoreCase);
        }

        [return: NotNullIfNotNull(nameof(riderPath))]
        private static string? CheckAndUpdatePath(string? riderPath)
        {
            if (File.Exists(riderPath))
            {
                return riderPath;
            }

            var allInfos = Utils.Locator.GetAllRiderPaths();
            if (allInfos.Length == 0)
                return null;

            // RiderPathLocator includes Rider and Fleet locations, prefer Rider when available.
            var preferredInfo = allInfos.LastOrDefault(info => IsRider(info.Path), allInfos[allInfos.Length - 1]);
            string newPath = preferredInfo.Path;

            var editorSettings = EditorInterface.Singleton.GetEditorSettings();
            editorSettings.SetSetting(EditorPathSettingName, newPath);
            Globals.EditorDef(EditorPathSettingName, newPath);
            return newPath;
        }

        public static void OpenFile(string slnPath, string scriptPath, int line, int column)
        {
            string? pathFromSettings = GetRiderPathFromSettings();
            string? path = CheckAndUpdatePath(pathFromSettings);
            if (string.IsNullOrEmpty(path))
            {
                GD.PushError($"Error when trying to run code editor: JetBrains Rider or Fleet. Could not find path to the editor.");
                return;
            }

            Utils.Opener.OpenFile(path, slnPath, scriptPath, line, column);
        }
    }
}
