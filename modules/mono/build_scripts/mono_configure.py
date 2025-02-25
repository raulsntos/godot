import os
import os.path
import shutil


def is_desktop(platform):
    return platform in ["windows", "macos", "linuxbsd"]


def is_unix_like(platform):
    return platform in ["macos", "linuxbsd", "android", "ios"]


def module_supports_tools_on(platform):
    return is_desktop(platform)


def configure(env, env_mono):
    if env.editor_build:
        if not module_supports_tools_on(env["platform"]):
            raise RuntimeError("This module does not currently support building for this platform for editor builds.")
        env_mono.Append(CPPDEFINES=["GD_MONO_HOT_RELOAD"])

    if not env.editor_build and env["platform"] == "web":
        env_mono.Append(CPPDEFINES=["MONOVM_ENABLED"])

        rid = get_rid(env["platform"], env["arch"])

        this_script_dir = os.path.dirname(os.path.realpath(__file__))
        module_dir = os.path.abspath(os.path.join(this_script_dir, os.pardir))
        get_runtime_pack_project_path = os.path.join(module_dir, "runtime", "GetRuntimePack")

        mono_runtime_copy_path = os.path.join(get_runtime_pack_project_path, "bin", "mono_runtime")
        try:
            shutil.rmtree(mono_runtime_copy_path)
        except FileNotFoundError:
            # It's fine if this directory doesn't exist.
            pass

        import subprocess
        exit_code = subprocess.call(
            [
                "dotnet", "publish",
                get_runtime_pack_project_path,
                "-r", rid,
                "--self-contained",
                "-c", "Release",
            ]
        )
        if exit_code != 0:
            raise RuntimeError("Couldn't retrieve Mono runtime pack for '" + rid + "'.")

        f = open(os.path.join(get_runtime_pack_project_path, "bin", rid, "runtime-pack-dir.txt"))
        mono_runtime_path = os.path.join(f.readline().strip(), "runtimes", rid, "native")
        shutil.copytree(mono_runtime_path, mono_runtime_copy_path)
        mono_runtime_path = mono_runtime_copy_path

        if env["platform"] == "web":
            # The default Mono runtime uses WASM exceptions.
            env.Append(CCFLAGS=["-fwasm-exceptions"])
            env.Append(LINKFLAGS=["-fwasm-exceptions"])

        env_mono.Prepend(CPPPATH=os.path.join(mono_runtime_path, "include", "mono-2.0"))

        env_mono.Append(LIBPATH=[mono_runtime_path])

        # Mono libraries.
        add_mono_library(env, "libmonosgen-2.0.a", mono_runtime_path)
        add_mono_library(env, "libmono-wasm-eh-wasm.a", mono_runtime_path)
        add_mono_library(env, "libmono-ee-interp.a", mono_runtime_path)
        add_mono_library(env, "libSystem.Native.a", mono_runtime_path)
        add_mono_library(env, "libSystem.Globalization.Native.a", mono_runtime_path)
        add_mono_library(env, "libSystem.IO.Compression.Native.a", mono_runtime_path)
        add_mono_library(env, "wasm-bundled-timezones.a", mono_runtime_path)
        add_mono_library(env, "libmono-wasm-simd.a", mono_runtime_path)
        add_mono_library(env, "libmono-icall-table.a", mono_runtime_path)
        add_mono_library(env, "libmono-profiler-browser.a", mono_runtime_path)
        add_mono_library(env, "libicuuc.a", mono_runtime_path)
        add_mono_library(env, "libicudata.a", mono_runtime_path)
        add_mono_library(env, "libicui18n.a", mono_runtime_path)
        add_mono_library(env, "libz.a", mono_runtime_path)

        # Mono components.
        add_mono_component(env, "debugger", mono_runtime_path)
        add_mono_component(env, "diagnostics_tracing", mono_runtime_path)
        add_mono_component(env, "hot_reload", mono_runtime_path)
        add_mono_component(env, "marshal-ilgen", mono_runtime_path)

        # WASM additional sources.
        env_thirdparty = env_mono.Clone()
        env_thirdparty.disable_warnings()
        env_thirdparty.Append(CPPDEFINES=["GEN_PINVOKE"])
        # env_thirdparty.Append(CPPDEFINES=["ENABLE_AOT", "EE_MODE_LLVMONLY_INTERP"])
        if env["dlink_enabled"]:
            env_thirdparty.Append(CPPDEFINES=["WASM_SUPPORTS_DLOPEN"])
        env_thirdparty.Prepend(CPPPATH=os.path.join(mono_runtime_path, "include", "wasm"))
        get_runtime_pack_project_includes = os.path.join(get_runtime_pack_project_path, "obj", "Release", "net9.0", rid, "wasm", "for-publish")
        env_thirdparty.Prepend(CPPPATH=os.path.join(get_runtime_pack_project_includes))
        env_thirdparty.add_source_files(
            env.modules_sources,
            [
                os.path.join(mono_runtime_path, "src", "runtime.c"),
                os.path.join(mono_runtime_path, "src", "driver.c"),
                os.path.join(mono_runtime_path, "src", "corebindings.c"),
                os.path.join(mono_runtime_path, "src", "pinvoke.c"),
            ],
        )
        # When exporting to the web platform, Godot uses `-fvisibility=hidden` to avoid exposing
        # too many symbols, otherwise the browser will not run it. We need to expose symbols
        # that may not be annotated, we have no control over the source files provided by .NET WASM.
        try:
            env_thirdparty["CCFLAGS"].remove("-fvisibility=hidden")
        except ValueError:
            # It's fine if the flag wasn't there.
            pass

        # .NET uses ES6 modules for their javascript bootstraping so make sure to use the right flags.
        env.Append(LINKFLAGS=["-s", "EXPORT_ES6=1"])
        env.Append(LINKFLAGS=["-s", "MODULARIZE=1"])
        env.AddJSLibraries([os.path.join(mono_runtime_path, "src", "es6", "dotnet.es6.lib.js")])


"""
Get the .NET runtime identifier for the given Godot platform and architecture names.
"""
def get_rid(platform: str, arch: str):
    # Web runtime identifier is always browser-wasm.
    if platform == "web":
        return "browser-wasm"

    # Platform renames.
    if platform == "macos":
        platform = "osx"
    elif platform == "linuxbsd":
        platform = "linux"

    # Arch renames.
    if arch == "x86_32":
        arch = "x86"
    elif arch == "x86_64":
        arch = "x64"
    elif arch == "arm32":
        arch = "arm"

    return platform + "-" + arch


"""
The mono library can have different names, usually `monosgen-2.0`
but may be renamed to `coreclr` in some platforms.
See: https://github.com/dotnet/runtime/issues/34202
"""
def get_mono_library_name(mono_runtime_path: str, platform: str):
    prefixes = ["lib"]
    extensions = [".so"]

    if platform == "windows":
        extensions = [".dll"]
        prefixes = [""]
    elif platform == "macos" or platform == "ios":
        extensions = ["dylib"]
        if platform == "ios":
            extensions.append(".a")
    elif platform == "android":
        extensions.append(".a")
    elif platform == "web":
        extensions = [".a"]

    for name in ["coreclr", "monosgen-2.0"]:
        for extension in extensions:
            if extension and not extension.startswith("."):
                extension = "." + extension
            for prefix in prefixes:
                if os.path.isfile(os.path.join(mono_runtime_path, prefix + name + extension)):
                    return name

    return None


"""
Link mono components statically (use the stubs to load them dynamically at runtime).
See: https://github.com/dotnet/runtime/blob/main/docs/design/mono/components.md
"""
def add_mono_component(env, name: str, mono_runtime_path: str, is_stub: bool = False):
    stub_suffix = "-stub" if is_stub else ""
    component_filename = "libmono-component-" + name + stub_suffix + "-static.a"
    add_mono_library(env, component_filename, mono_runtime_path)


"""
Link mono library archive (.a) statically.
"""
def add_mono_library(env, filename: str, mono_runtime_path: str):
    assert(filename.endswith(".a"))
    env.Append(
        LINKFLAGS=[
            "-Wl,-whole-archive",
            os.path.join(mono_runtime_path, filename),
            "-Wl,-no-whole-archive",
        ]
    )
