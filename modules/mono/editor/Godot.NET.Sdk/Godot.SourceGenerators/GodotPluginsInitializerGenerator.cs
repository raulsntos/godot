using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.Text;

namespace Godot.SourceGenerators
{
    [Generator]
    public class GodotPluginsInitializerGenerator : ISourceGenerator
    {
        public void Initialize(GeneratorInitializationContext context)
        {
        }

        public void Execute(GeneratorExecutionContext context)
        {
            if (context.IsGodotToolsProject() || context.IsGodotSourceGeneratorDisabled("GodotPluginsInitializer"))
                return;

            string source =
                @"using System;
using System.Runtime.InteropServices;
using Godot.Bridge;
using Godot.NativeInterop;

namespace GodotPlugins.Game
{
    internal static partial class Main
    {
        [UnmanagedCallersOnly(EntryPoint = ""godotsharp_game_main_init"")]
        private static godot_bool InitializeFromGameProject(IntPtr godotDllHandle, IntPtr outManagedCallbacks,
            IntPtr unmanagedCallbacks, int unmanagedCallbacksSize)
        {
            // This is just temporary for testing something before actually initializing Godot,
            // since initialization currently fails.
            var initMethod = typeof(Main).GetMethod(""TestMethod"", global::System.Reflection.BindingFlags.Public | global::System.Reflection.BindingFlags.NonPublic | global::System.Reflection.BindingFlags.Static);
            if (initMethod != null)
            {
                global::System.Console.WriteLine(""TestMethod found"");
                object ret = initMethod.Invoke(null, new object[] { godotDllHandle, outManagedCallbacks, unmanagedCallbacks, unmanagedCallbacksSize });
                if ((bool)ret)
                {
                    return godot_bool.False;
                }
            }
            else
            {
                global::System.Console.WriteLine(""TestMethod not found"");
            }

            try
            {
                DllImportResolver dllImportResolver = new GodotDllImportResolver(godotDllHandle).OnResolveDllImport;

                var coreApiAssembly = typeof(global::Godot.GodotObject).Assembly;

                NativeLibrary.SetDllImportResolver(coreApiAssembly, dllImportResolver);

                NativeFuncs.Initialize(unmanagedCallbacks, unmanagedCallbacksSize);

                ManagedCallbacks.Create(outManagedCallbacks);

                ScriptManagerBridge.LookupScriptsInAssembly(typeof(global::GodotPlugins.Game.Main).Assembly);

                return godot_bool.True;
            }
            catch (Exception e)
            {
                global::System.Console.Error.WriteLine(e);
                return false.ToGodotBool();
            }
        }
    }
}
";

            context.AddSource("GodotPlugins.Game.generated",
                SourceText.From(source, Encoding.UTF8));
        }
    }
}
