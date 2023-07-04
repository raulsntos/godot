using Godot;
using GodotTools.Core;
using GodotTools.Export;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using GodotTools.Build;
using GodotTools.Ides;
using GodotTools.Inspector;
using GodotTools.Internals;
using GodotTools.ProjectEditor;
using JetBrains.Annotations;
using static GodotTools.Internals.Globals;
using Environment = System.Environment;
using File = GodotTools.Utils.File;
using OS = GodotTools.Utils.OS;
using Path = System.IO.Path;

namespace GodotTools
{
    public partial class GodotSharpEditor : EditorPlugin, ISerializationListener
    {
        public static class Settings
        {
            public const string ExternalEditor = "dotnet/editor/external_editor";
            public const string CustomExecPath = "dotnet/editor/custom_exec_path";
            public const string CustomExecPathArgs = "dotnet/editor/custom_exec_path_args";
            public const string VerbosityLevel = "dotnet/build/verbosity_level";
            public const string NoConsoleLogging = "dotnet/build/no_console_logging";
            public const string CreateBinaryLog = "dotnet/build/create_binary_log";
            public const string ProblemsLayout = "dotnet/build/problems_layout";
        }

        private EditorSettings _editorSettings;

        private PopupMenu _menuPopup;

        private AcceptDialog _errorDialog;
        private ConfirmationDialog _confirmCreateSlnDialog;

        private Button _bottomPanelBtn;
        private Button _toolBarBuildButton;

        // TODO Use WeakReference once we have proper serialization.
        private WeakRef _exportPluginWeak;
        private WeakRef _inspectorPluginWeak;

        public IdeManager IdeManager { get; private set; }

        public MSBuildPanel MSBuildPanel { get; private set; }

        public bool SkipBuildBeforePlaying { get; set; } = false;

        public DateTime LastValidBuildDateTime { get; private set; }

        [UsedImplicitly]
        private bool CreateProjectSolutionIfNeeded()
        {
            if (!File.Exists(GodotSharpDirs.ProjectSlnPath) || !File.Exists(GodotSharpDirs.ProjectCsProjPath))
            {
                return CreateProjectSolution();
            }

            return true;
        }

        private bool CreateProjectSolution()
        {
            string errorMessage = null;
            using (var pr = new EditorProgress("create_csharp_solution", "Generating solution...".TTR(), 2))
            {
                pr.Step("Generating C# project...".TTR());

                string csprojDir = Path.GetDirectoryName(GodotSharpDirs.ProjectCsProjPath);
                string slnDir = Path.GetDirectoryName(GodotSharpDirs.ProjectSlnPath);
                string name = GodotSharpDirs.ProjectAssemblyName;
                string guid = CsProjOperations.GenerateGameProject(csprojDir, name);

                if (guid.Length > 0)
                {
                    var solution = new DotNetSolution(name)
                    {
                        DirectoryPath = slnDir
                    };

                    var projectInfo = new DotNetSolution.ProjectInfo
                    {
                        Guid = guid,
                        PathRelativeToSolution = Path.GetRelativePath(slnDir, GodotSharpDirs.ProjectCsProjPath),
                        Configs = new List<string> { "Debug", "ExportDebug", "ExportRelease" }
                    };

                    solution.AddNewProject(name, projectInfo);

                    try
                    {
                        solution.Save();
                    }
                    catch (IOException e)
                    {
                        errorMessage = "Failed to save solution. Exception message: ".TTR() + e.Message;
                    }
                }
                else
                {
                    errorMessage = "Failed to create C# project.".TTR();
                }
            }

            if (!string.IsNullOrEmpty(errorMessage))
            {
                ShowErrorDialog(errorMessage);
                return false;
            }

            _ShowDotnetFeatures();
            return true;
        }

        private void _ShowDotnetFeatures()
        {
            _bottomPanelBtn.Show();
            _toolBarBuildButton.Show();
        }

        private void _MenuOptionPressed(long id)
        {
            switch ((MenuOptions)id)
            {
                case MenuOptions.CreateSln:
                {
                    if (File.Exists(GodotSharpDirs.ProjectSlnPath) || File.Exists(GodotSharpDirs.ProjectCsProjPath))
                    {
                        ShowConfirmCreateSlnDialog();
                    }
                    else
                    {
                        CreateProjectSolution();
                    }
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException(nameof(id), id, "Invalid menu option");
            }
        }

        private void BuildProjectPressed()
        {
            if (!File.Exists(GodotSharpDirs.ProjectCsProjPath))
            {
                if (!CreateProjectSolution())
                    return; // Failed to create project.
            }

            Instance.MSBuildPanel.BuildProject();
        }

        private enum MenuOptions
        {
            CreateSln,
        }

        public void ShowErrorDialog(string message, string title = "Error")
        {
            _errorDialog.Title = title;
            _errorDialog.DialogText = message;
            EditorInterface.Singleton.PopupDialogCentered(_errorDialog);
        }

        public void ShowConfirmCreateSlnDialog()
        {
            _confirmCreateSlnDialog.Title = "C# solution already exists. This will override the existing C# project file, any manual changes will be lost.".TTR();
            _confirmCreateSlnDialog.DialogText = "Create C# solution".TTR();
            EditorInterface.Singleton.PopupDialogCentered(_confirmCreateSlnDialog);
        }

        [UsedImplicitly]
        public Error OpenInExternalEditor(Script script, int line, int column)
        {
            var editorId = _editorSettings.GetSetting(Settings.ExternalEditor).As<ExternalEditorId>();
            return IdeManager.OpenInExternalEditor(editorId, script, line, column);
        }

        [UsedImplicitly]
        public bool OverridesExternalEditor()
        {
            return _editorSettings.GetSetting(Settings.ExternalEditor).As<ExternalEditorId>() != ExternalEditorId.None;
        }

        public override bool _Build()
        {
            return BuildManager.EditorBuildCallback();
        }

        private void ApplyNecessaryChangesToSolution()
        {
            try
            {
                // Migrate solution from old configuration names to: Debug, ExportDebug and ExportRelease
                DotNetSolution.MigrateFromOldConfigNames(GodotSharpDirs.ProjectSlnPath);

                var msbuildProject = ProjectUtils.Open(GodotSharpDirs.ProjectCsProjPath)
                                     ?? throw new InvalidOperationException("Cannot open C# project.");

                // NOTE: The order in which changes are made to the project is important

                // Migrate to MSBuild project Sdks style if using the old style
                ProjectUtils.MigrateToProjectSdksStyle(msbuildProject, GodotSharpDirs.ProjectAssemblyName);

                ProjectUtils.EnsureGodotSdkIsUpToDate(msbuildProject);

                if (msbuildProject.HasUnsavedChanges)
                {
                    // Save a copy of the project before replacing it
                    FileUtils.SaveBackupCopy(GodotSharpDirs.ProjectCsProjPath);

                    msbuildProject.Save();
                }
            }
            catch (Exception e)
            {
                GD.PushError(e.ToString());
            }
        }

        private void UpdateLastValidBuildDateTime()
        {
            var dllName = $"{GodotSharpDirs.ProjectAssemblyName}.dll";
            var path = Path.Combine(GodotSharpDirs.ProjectBaseOutputPath, "Debug", dllName);
            LastValidBuildDateTime = File.GetLastWriteTime(path);
        }

        private void BuildFinished(BuildResult buildResult)
        {
            if (buildResult == BuildResult.Success)
            {
                UpdateLastValidBuildDateTime();
            }
        }

        private void BuildStateChanged()
        {
            if (_bottomPanelBtn != null)
                _bottomPanelBtn.Icon = MSBuildPanel.GetBuildStateIcon();
        }

        public override void _EnablePlugin()
        {
            base._EnablePlugin();

            UpdateLastValidBuildDateTime();

            ProjectSettings.SettingsChanged += GodotSharpDirs.DetermineProjectLocation;

            if (Instance != null)
                throw new InvalidOperationException();
            Instance = this;

            var dotNetSdkSearchVersion = Environment.Version;

            // First we try to find the .NET Sdk ourselves to make sure we get the
            // correct version first, otherwise pick the latest.
            if (DotNetFinder.TryFindDotNetSdk(dotNetSdkSearchVersion, out var sdkVersion, out string sdkPath))
            {
                if (Godot.OS.IsStdOutVerbose())
                    Console.WriteLine($"Found .NET Sdk version '{sdkVersion}': {sdkPath}");

                ProjectUtils.MSBuildLocatorRegisterMSBuildPath(sdkPath);
            }
            else
            {
                try
                {
                    ProjectUtils.MSBuildLocatorRegisterLatest(out sdkVersion, out sdkPath);
                    if (Godot.OS.IsStdOutVerbose())
                        Console.WriteLine($"Found .NET Sdk version '{sdkVersion}': {sdkPath}");
                }
                catch (InvalidOperationException e)
                {
                    if (Godot.OS.IsStdOutVerbose())
                        GD.PrintErr(e.ToString());
                    GD.PushError($".NET Sdk not found. The required version is '{dotNetSdkSearchVersion}'.");
                }
            }

            var editorBaseControl = EditorInterface.Singleton.GetBaseControl();

            _editorSettings = EditorInterface.Singleton.GetEditorSettings();

            _errorDialog = new AcceptDialog();
            _errorDialog.SetUnparentWhenInvisible(true);

            _confirmCreateSlnDialog = new ConfirmationDialog();
            _confirmCreateSlnDialog.SetUnparentWhenInvisible(true);
            _confirmCreateSlnDialog.Confirmed += () => CreateProjectSolution();

            MSBuildPanel = new MSBuildPanel();
            MSBuildPanel.BuildStateChanged += BuildStateChanged;
            _bottomPanelBtn = AddControlToBottomPanel(MSBuildPanel, "MSBuild".TTR());

            AddChild(new HotReloadAssemblyWatcher { Name = "HotReloadAssemblyWatcher" });

            _menuPopup = new PopupMenu
            {
                Name = "CSharpTools",
            };
            _menuPopup.Hide();

            AddToolSubmenuItem("C#", _menuPopup);

            _toolBarBuildButton = new Button
            {
                Flat = true,
                Icon = EditorInterface.Singleton.GetEditorTheme().GetIcon("BuildCSharp", "EditorIcons"),
                FocusMode = Control.FocusModeEnum.None,
                Shortcut = EditorDefShortcut("mono/build_solution", "Build Project".TTR(), (Key)KeyModifierMask.MaskAlt | Key.B),
                ShortcutInTooltip = true,
            };
            EditorShortcutOverride("mono/build_solution", "macos", (Key)KeyModifierMask.MaskMeta | (Key)KeyModifierMask.MaskCtrl | Key.B);

            _toolBarBuildButton.Pressed += BuildProjectPressed;
            Internal.EditorPlugin_AddControlToEditorRunBar(_toolBarBuildButton);
            // Move Build button so it appears to the left of the Play button.
            _toolBarBuildButton.GetParent().MoveChild(_toolBarBuildButton, 0);

            if (File.Exists(GodotSharpDirs.ProjectCsProjPath))
            {
                ApplyNecessaryChangesToSolution();
            }
            else
            {
                _bottomPanelBtn.Hide();
                _toolBarBuildButton.Hide();
            }
            _menuPopup.AddItem("Create C# solution".TTR(), (int)MenuOptions.CreateSln);

            _menuPopup.IdPressed += _MenuOptionPressed;

            // External editor settings
            EditorDef(Settings.ExternalEditor, Variant.From(ExternalEditorId.None));
            EditorDef(Settings.CustomExecPath, "");
            EditorDef(Settings.CustomExecPathArgs, "");
            EditorDef(Settings.VerbosityLevel, Variant.From(VerbosityLevelId.Normal));
            EditorDef(Settings.NoConsoleLogging, false);
            EditorDef(Settings.CreateBinaryLog, false);
            EditorDef(Settings.ProblemsLayout, Variant.From(BuildProblemsView.ProblemsLayout.Tree));

            string settingsHintStr = "Disabled";

            if (OS.IsWindows)
            {
                settingsHintStr += $",Visual Studio:{(int)ExternalEditorId.VisualStudio}" +
                                   $",MonoDevelop:{(int)ExternalEditorId.MonoDevelop}" +
                                   $",Visual Studio Code:{(int)ExternalEditorId.VSCode}" +
                                   $",JetBrains Rider and Fleet:{(int)ExternalEditorId.Rider}" +
                                   $",Custom:{(int)ExternalEditorId.CustomEditor}";
            }
            else if (OS.IsMacOS)
            {
                settingsHintStr += $",Visual Studio for Mac:{(int)ExternalEditorId.VisualStudioForMac}" +
                                   $",MonoDevelop:{(int)ExternalEditorId.MonoDevelop}" +
                                   $",Visual Studio Code:{(int)ExternalEditorId.VSCode}" +
                                   $",JetBrains Rider and Fleet:{(int)ExternalEditorId.Rider}" +
                                   $",Custom:{(int)ExternalEditorId.CustomEditor}";
            }
            else if (OS.IsUnixLike)
            {
                settingsHintStr += $",MonoDevelop:{(int)ExternalEditorId.MonoDevelop}" +
                                   $",Visual Studio Code:{(int)ExternalEditorId.VSCode}" +
                                   $",JetBrains Rider and Fleet:{(int)ExternalEditorId.Rider}" +
                                   $",Custom:{(int)ExternalEditorId.CustomEditor}";
            }

            _editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
            {
                ["type"] = (int)Variant.Type.Int,
                ["name"] = Settings.ExternalEditor,
                ["hint"] = (int)PropertyHint.Enum,
                ["hint_string"] = settingsHintStr
            });

            _editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
            {
                ["type"] = (int)Variant.Type.String,
                ["name"] = Settings.CustomExecPath,
                ["hint"] = (int)PropertyHint.GlobalFile,
            });

            _editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
            {
                ["type"] = (int)Variant.Type.String,
                ["name"] = Settings.CustomExecPathArgs,
            });
            _editorSettings.SetInitialValue(Settings.CustomExecPathArgs, "{file}", false);

            var verbosityLevels = Enum.GetValues<VerbosityLevelId>().Select(level => $"{Enum.GetName(level)}:{(int)level}");
            _editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
            {
                ["type"] = (int)Variant.Type.Int,
                ["name"] = Settings.VerbosityLevel,
                ["hint"] = (int)PropertyHint.Enum,
                ["hint_string"] = string.Join(",", verbosityLevels),
            });

            _editorSettings.AddPropertyInfo(new Godot.Collections.Dictionary
            {
                ["type"] = (int)Variant.Type.Int,
                ["name"] = Settings.ProblemsLayout,
                ["hint"] = (int)PropertyHint.Enum,
                ["hint_string"] = "View as List,View as Tree",
            });

            OnSettingsChanged();
            _editorSettings.SettingsChanged += OnSettingsChanged;

            // Export plugin
            var exportPlugin = new ExportPlugin();
            AddExportPlugin(exportPlugin);
            _exportPluginWeak = WeakRef(exportPlugin);

            // Inspector plugin
            var inspectorPlugin = new InspectorPlugin();
            AddInspectorPlugin(inspectorPlugin);
            _inspectorPluginWeak = WeakRef(inspectorPlugin);
            BuildManager.BuildFinished += BuildFinished;

            BuildManager.Initialize();
            IdeManager = new IdeManager();
        }

        public override void _DisablePlugin()
        {
            base._DisablePlugin();

            _editorSettings.SettingsChanged -= OnSettingsChanged;

            // Custom signals aren't automatically disconnected currently.
            MSBuildPanel.BuildStateChanged -= BuildStateChanged;
            BuildManager.BuildFinished -= BuildFinished;
        }

        public override void _ExitTree()
        {
            _errorDialog?.QueueFree();
            _confirmCreateSlnDialog?.QueueFree();
        }

        private void OnSettingsChanged()
        {
            // We want to force NoConsoleLogging to true when the VerbosityLevel is at Detailed or above.
            // At that point, there's so much info logged that it doesn't make sense to display it in
            // the tiny editor window, and it'd make the editor hang or crash anyway.
            var verbosityLevel = _editorSettings.GetSetting(Settings.VerbosityLevel).As<VerbosityLevelId>();
            var hideConsoleLog = (bool)_editorSettings.GetSetting(Settings.NoConsoleLogging);
            if (verbosityLevel >= VerbosityLevelId.Detailed && !hideConsoleLog)
                _editorSettings.SetSetting(Settings.NoConsoleLogging, Variant.From(true));
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (IsInstanceValid(_exportPluginWeak))
                {
                    // We need to dispose our export plugin before the editor destroys EditorSettings.
                    // Otherwise, if the GC disposes it at a later time, EditorExportPlatformAndroid
                    // will be freed after EditorSettings already was, and its device polling thread
                    // will try to access the EditorSettings singleton, resulting in null dereferencing.
                    (_exportPluginWeak.GetRef().AsGodotObject() as ExportPlugin)?.Dispose();

                    _exportPluginWeak.Dispose();
                }

                if (IsInstanceValid(_inspectorPluginWeak))
                {
                    (_inspectorPluginWeak.GetRef().AsGodotObject() as InspectorPlugin)?.Dispose();

                    _inspectorPluginWeak.Dispose();
                }

                IdeManager.Dispose();
            }

            base.Dispose(disposing);
        }

        public void OnBeforeSerialize()
        {
        }

        public void OnAfterDeserialize()
        {
            Instance = this;
            IdeManager = new IdeManager();
        }

        // Singleton

        public static GodotSharpEditor Instance { get; private set; }

        [UsedImplicitly]
        private static IntPtr InternalCreateInstance(IntPtr unmanagedCallbacks, int unmanagedCallbacksSize)
        {
            Internal.Initialize(unmanagedCallbacks, unmanagedCallbacksSize);
            return new GodotSharpEditor().NativeInstance;
        }
    }
}
