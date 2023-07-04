using System;
using Godot;
using GodotTools.IdeMessaging;
using GodotTools.Ides.CustomEditor;
using GodotTools.Ides.MonoDevelop;
using GodotTools.Ides.Rider;
using GodotTools.Ides.VisualStudio;
using GodotTools.Ides.VisualStudioCode;
using GodotTools.Ides.VisualStudioForMac;
using GodotTools.Internals;

#nullable enable

namespace GodotTools.Ides
{
    public sealed class IdeManager : IDisposable
    {
        private MessagingServer? _messagingServer;

        private ExternalEditorManager? _currentEditorManager;

        public IdeManager()
        {
            _ = GetRunningOrNewServer();
        }

        public MessagingServer GetRunningOrNewServer()
        {
            if (_messagingServer != null && !_messagingServer.IsDisposed)
                return _messagingServer;

            _messagingServer?.Dispose();
            _messagingServer = new MessagingServer(OS.GetExecutablePath(),
                ProjectSettings.GlobalizePath(GodotSharpDirs.ResMetadataDir),
                new GodotLogger());

            _ = _messagingServer.Listen();

            return _messagingServer;
        }

        private ExternalEditorManager GetExternalEditorManager(ExternalEditorId editorId)
        {
            return editorId switch
            {
                ExternalEditorId.VisualStudio when OperatingSystem.IsWindows() =>
                    GetExternalEditorManagerOfType<VisualStudioManager>(),
                ExternalEditorId.VisualStudio when !OperatingSystem.IsWindows() =>
                    throw new PlatformNotSupportedException("Visual Studio not supported on this platform."),

                ExternalEditorId.VisualStudioForMac when OperatingSystem.IsMacOS() =>
                    GetExternalEditorManagerOfType<VisualStudioForMacManager>(),
                ExternalEditorId.VisualStudioForMac when !OperatingSystem.IsMacOS() =>
                    throw new PlatformNotSupportedException("Visual Studio for Mac not supported on this platform."),

                ExternalEditorId.MonoDevelop =>
                    GetExternalEditorManagerOfType<MonoDevelopManager>(),

                ExternalEditorId.VSCode =>
                    GetExternalEditorManagerOfType<VisualStudioCodeManager>(),

                ExternalEditorId.Rider =>
                    GetExternalEditorManagerOfType<RiderManager>(),

                ExternalEditorId.CustomEditor =>
                    GetExternalEditorManagerOfType<CustomEditorManager>(),

                _ => throw new ArgumentException($"IDE manager does not exist for '{editorId}'.", nameof(editorId)),
            };

            ExternalEditorManager GetExternalEditorManagerOfType<TEditorManager>() where TEditorManager : ExternalEditorManager, new()
            {
                if (_currentEditorManager is not TEditorManager)
                {
                    (_currentEditorManager as IDisposable)?.Dispose();
                    _currentEditorManager = new TEditorManager();
                }

                return _currentEditorManager;
            }
        }

        public Error OpenInExternalEditor(ExternalEditorId editorId, Script script, int line, int column)
        {
            if (editorId == ExternalEditorId.None)
            {
                // Not an error. Tells the caller to fallback to the
                // global external editor settings or the built-in editor.
                return Error.Unavailable;
            }

            try
            {
                var manager = GetExternalEditorManager(editorId);
                return manager.Launch(script, line, column);
            }
            catch (Exception e)
            {
                GD.PushError($"Error when trying to run code editor: {editorId}. Exception message: '{e.Message}'");
                return Error.Failed;
            }
        }

        public void Dispose()
        {
            (_currentEditorManager as IDisposable)?.Dispose();
        }

        private class GodotLogger : ILogger
        {
            public void LogDebug(string message)
            {
                if (OS.IsStdOutVerbose())
                    Console.WriteLine(message);
            }

            public void LogInfo(string message)
            {
                if (OS.IsStdOutVerbose())
                    Console.WriteLine(message);
            }

            public void LogWarning(string message)
            {
                GD.PushWarning(message);
            }

            public void LogError(string message)
            {
                GD.PushError(message);
            }

            public void LogError(string message, Exception e)
            {
                GD.PushError($"{message}\n{e}");
            }
        }
    }
}
