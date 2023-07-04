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
    public class RiderManager : ExternalEditorManager
    {
        private static readonly RiderPathLocator RiderPathLocator;
        private static readonly RiderFileOpener RiderFileOpener;

        private static string? _riderPath;

        static RiderManager()
        {
            var riderLocatorEnvironment = new RiderLocatorEnvironment();
            RiderPathLocator = new RiderPathLocator(riderLocatorEnvironment);
            RiderFileOpener = new RiderFileOpener(riderLocatorEnvironment);
        }

        public override Error Launch(Script script, int line, int column)
        {
            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);

            if (!File.Exists(_riderPath))
            {
                // Try to search it again if it wasn't found last time or if it was removed from its location.
                if (!TryGetRiderPath(out _riderPath))
                {
                    GD.PushError("Cannot find code editor: JetBrains Rider");
                    return Error.FileNotFound;
                }
            }

            RiderFileOpener.OpenFile(_riderPath, GodotSharpDirs.ProjectSlnPath, scriptPath, line + 1, column);
            return Error.Ok;
        }

        private static bool TryGetRiderPath([NotNullWhen(true)] out string? path)
        {
            var allInfos = RiderPathLocator.GetAllRiderPaths();
            if (allInfos.Length == 0)
            {
                path = null;
                return false;
            }

            var riderInfos = allInfos
                .Where(info => IsRider(info.Path))
                .ToArray();

            path = riderInfos.Length > 0
                ? riderInfos[riderInfos.Length - 1].Path
                : allInfos[allInfos.Length - 1].Path;
            return true;
        }

        private static bool IsRider([NotNullWhen(true)] string? path)
        {
            if (string.IsNullOrEmpty(path))
                return false;

            if (path.IndexOfAny(Path.GetInvalidPathChars()) != -1)
                return false;

            return Path.GetFileName(path).StartsWith("rider", StringComparison.OrdinalIgnoreCase);
        }
    }
}
