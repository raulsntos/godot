using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Threading.Tasks;
using Godot;
using GodotTools.IdeMessaging.Requests;
using GodotTools.Internals;
using OS = GodotTools.Utils.OS;

#nullable enable

namespace GodotTools.Ides.VisualStudioForMac
{
    public class VisualStudioForMacManager : ExternalEditorManager, IDisposable
    {
        private const string VSExecutableName = "VisualStudio";
        private const string VSBundleId = "com.microsoft.visual-studio";
        private const string VSEditorIdentity = "VisualStudioForMac";

        private string? _vsPath;

        private Process? _process;
        private DateTime _launchTime;

        [MemberNotNullWhen(true, nameof(_process))]
        private bool IsRunning => _process != null && !_process.HasExited;

        public override Error Launch(Script script, int line, int column)
        {
            if (!OperatingSystem.IsMacOS())
            {
                GD.PushError("Visual Studio for Mac not supported on this platform.");
                return Error.Failed;
            }

            if (IsRunning)
            {
                if (!IdeManager.GetRunningOrNewServer().IsAnyConnected(VSEditorIdentity))
                {
                    // After launch we wait up to 30 seconds for the IDE to connect to our messaging server.
                    var waitAfterLaunch = TimeSpan.FromSeconds(30);
                    var timeSinceLaunch = DateTime.Now - _launchTime;
                    if (timeSinceLaunch > waitAfterLaunch)
                    {
                        _process.Dispose();
                    }
                    else
                    {
                        goto RequestOpenFile;
                    }
                }
                else
                {
                    goto RequestOpenFile;
                }
            }

            string command;

            var args = new List<string>();

            if (OperatingSystem.IsMacOS() && Internal.IsMacOSAppBundleInstalled(VSBundleId))
            {
                command = "/usr/bin/open";

                args.Add("-b");
                args.Add(VSBundleId);

                // The 'open' process must wait until the application finishes.
                args.Add("--wait-apps");

                args.Add("--args");
            }
            else
            {
                if (string.IsNullOrEmpty(_vsPath) || !File.Exists(_vsPath))
                {
                    // Try to search it again if it wasn't found last time or if it was removed from its location.
                    _vsPath = OS.PathWhich(VSExecutableName);
                }

                if (string.IsNullOrEmpty(_vsPath))
                {
                    GD.PushError("Cannot find code editor: Visual Studio for Mac");
                    return Error.FileNotFound;
                }

                command = _vsPath;
            }

            args.Add("--ipc-tcp");

            args.Add(Path.GetFullPath(GodotSharpDirs.ProjectSlnPath));

            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);
            args.Add(line >= 0 ? $"{scriptPath};{line + 1};{column}" : scriptPath);

            _launchTime = DateTime.Now;
            _process = Process.Start(command, args);

        RequestOpenFile:
            _ = RequestOpenFileAsync();
            return Error.Ok;

            async Task RequestOpenFileAsync()
            {
                const int millisecondsTimeout = 10_000;
                var timeoutTask = Task.Delay(millisecondsTimeout);
                var completedTask = await Task.WhenAny(timeoutTask, IdeManager.GetRunningOrNewServer().AwaitClientConnected(VSEditorIdentity));

                if (completedTask == timeoutTask)
                {
                    GD.PushError("Could not connect to code editor after 10 seconds: Visual Studio for Mac.");
                    return;
                }

                RequestOpenFile(script, line, column);
            }
        }

        private void RequestOpenFile(Script script, int line, int column)
        {
            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);

            var openFileRequest = new OpenFileRequest()
            {
                File = scriptPath,
                Line = line >= 0 ? line + 1 : null,
                Column = line >= 0 ? column : null,
            };

            IdeManager.GetRunningOrNewServer().BroadcastRequest<OpenFileResponse>(VSEditorIdentity, openFileRequest);
        }

        public void Dispose()
        {
            _process?.Dispose();
        }
    }
}
