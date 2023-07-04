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

namespace GodotTools.Ides.MonoDevelop
{
    public class MonoDevelopManager : ExternalEditorManager, IDisposable
    {
        private static readonly string MonoDevelopExecutableName =
            OperatingSystem.IsWindows() ? "MonoDevelop.exe" : "monodevelop";
        private const string MonoDevelopEditorIdentity = "MonoDevelop";

        private string? _monoDevelopPath;

        private Process? _process;
        private DateTime _launchTime;

        [MemberNotNullWhen(true, nameof(_process))]
        private bool IsRunning => _process != null && !_process.HasExited;

        public override Error Launch(Script script, int line, int column)
        {
            if (IsRunning)
            {
                if (!IdeManager.GetRunningOrNewServer().IsAnyConnected(MonoDevelopEditorIdentity))
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

            if (string.IsNullOrEmpty(_monoDevelopPath) || !File.Exists(_monoDevelopPath))
            {
                // Try to search it again if it wasn't found last time or if it was removed from its location.
                _monoDevelopPath = OS.PathWhich(MonoDevelopExecutableName);
            }

            if (string.IsNullOrEmpty(_monoDevelopPath))
            {
                GD.PushError("Cannot find code editor: MonoDevelop");
                return Error.FileNotFound;
            }

            string scriptPath = ProjectSettings.GlobalizePath(script.ResourcePath);

            string command = _monoDevelopPath;

            var args = new List<string>
            {
                "--ipc-tcp",
                Path.GetFullPath(GodotSharpDirs.ProjectSlnPath),
                line >= 0 ? $"{scriptPath};{line + 1};{column}" : scriptPath,
            };

            _launchTime = DateTime.Now;
            _process = Process.Start(command, args);

        RequestOpenFile:
            _ = RequestOpenFileAsync();
            return Error.Ok;

            async Task RequestOpenFileAsync()
            {
                const int millisecondsTimeout = 10_000;
                var timeoutTask = Task.Delay(millisecondsTimeout);
                var completedTask = await Task.WhenAny(timeoutTask, IdeManager.GetRunningOrNewServer().AwaitClientConnected(MonoDevelopEditorIdentity));

                if (completedTask == timeoutTask)
                {
                    GD.PushError("Could not connect to code editor after 10 seconds: MonoDevelop.");
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

            IdeManager.GetRunningOrNewServer().BroadcastRequest<OpenFileResponse>(MonoDevelopEditorIdentity, openFileRequest);
        }

        public void Dispose()
        {
            _process?.Dispose();
        }
    }
}
