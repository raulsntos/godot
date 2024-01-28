/**************************************************************************/
/*  dotnet_module.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "dotnet_module.h"

#include "utils/dirs_utils.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "./editor/RestoreEditorPackages.proj.gen.h"
#include "runtime/hostfxr/hostfxr_dotnet_runtime.h"

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "editor/settings/editor_settings.h"
#endif

namespace DotNet {

DotNetModule *DotNetModule::singleton = nullptr;

DotNetRuntimeManager *DotNetModule::runtime_manager = nullptr;

DotNetModule *DotNetModule::get_singleton() {
	return singleton;
}

bool DotNetModule::is_initialized() const {
	return initialized;
}

bool DotNetModule::should_initialize() {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_project_manager_hint()) {
		// Never initialize the module in the project manager.
		return false;
	}
	// If we can find a C# project or solution in the workspace,
	// assume the Godot project uses C# and needs to initialize the module.
	return FileAccess::exists(Dirs::get_project_csproj_path()) || FileAccess::exists(Dirs::get_project_solution_path());
#else
	// The exported project was built with .NET support,
	// so it needs to initialize the module.
	return OS::get_singleton()->has_feature("dotnet");
#endif
}

void DotNetModule::initialize() {
	print_verbose(".NET: Initializing module.");

	runtime_manager = memnew(DotNetRuntimeManager);
	CRASH_COND(runtime_manager == nullptr);

	// There are 3 possible cases here:
	// 1. Exported project.
	//    Doesn't need the editor integration, it loads assemblies without 'Godot.PluginLoader'.
	// 2. Editor build running outside the editor.
	//    Needs the 'Godot.PluginLoader' assembly restored from the editor packages,
	//    but should not load the 'Godot.EditorIntegration' assembly.
	// 3. Editor build running inside the editor.
	//    Needs the 'Godot.PluginLoader' assembly restored from the editor packages,
	//    and should load the 'Godot.EditorIntegration' assembly.
#ifdef TOOLS_ENABLED
	String found_dotnet_sdk_path;
	if (!HostFxrDotNetRuntime::try_find_dotnet_sdk(found_dotnet_sdk_path)) {
		OS::get_singleton()->alert(TTR("Could not find a .NET SDK installation required for .NET support.\nPlease install the .NET SDK from https://get.dot.net and restart Godot."), TTR(".NET SDK not found"));
		CRASH_NOW_MSG(".NET: .NET SDK installation not found.");
	}

	print_verbose(vformat(".NET: .NET SDK installation found at '%s'.", found_dotnet_sdk_path));

	if (Engine::get_singleton()->is_editor_hint()) {
		const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
		if (!try_restore_editor_packages(editor_assemblies_dir)) {
			OS::get_singleton()->alert(TTR("Failed to restore .NET editor integration. Check the console output for more details."), TTR("Failed to restore .NET editor integration"));
			CRASH_NOW_MSG(".NET: Failed to restore editor integration.");
		}
		if (!runtime_manager->try_load_extension("Godot.EditorIntegration", editor_assemblies_dir)) {
			// If the editor integration could not be loaded, .NET-related functionality can't be enabled
			// (i.e.: .NET assemblies can't be loaded). Show an message to users to try to explain and crash
			// because we can't recover from this error.
			OS::get_singleton()->alert(TTR("Failed to load .NET editor integration. Check the console output for more details."), TTR("Failed to load .NET editor integration"));
			CRASH_NOW_MSG(".NET: Failed to load editor integration.");
		}
	}
#endif

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	bool should_load_project_assembly = true;

#ifdef TOOLS_ENABLED
	// Listen for changes to the assembly name setting so we can update the loaded assembly accordingly.
	ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &DotNetModule::_on_project_settings_changed));

	fs_watcher.instantiate();
	fs_watcher->set_path(assemblies_dir.path_join(assembly_name + ".dll"));
	fs_watcher->callable = callable_mp(this, &DotNetModule::_on_project_assembly_changed);
	callable_mp(this, &DotNetModule::_start_fs_watcher).call_deferred();

	should_load_project_assembly = FileAccess::exists(fs_watcher->get_path());
#endif

	if (should_load_project_assembly) {
		if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
			WARN_PRINT(".NET: Failed to load assembly '" + assembly_name + "'.");
		}
	}

	initialized = true;
	print_verbose(".NET: Module initialized.");
}

#ifdef TOOLS_ENABLED
void DotNetModule::register_editor_settings() {
	String external_editor_hint_string = "Disabled:0";
#ifdef WINDOWS_ENABLED
	external_editor_hint_string += ",Visual Studio:1";
#endif
#ifdef MACOS_ENABLED
	external_editor_hint_string += ",Visual Studio:2";
#endif
#ifdef UNIX_ENABLED
	external_editor_hint_string += ",MonoDevelop:3";
	external_editor_hint_string += ",Visual Studio Code and VSCodium:4";
	external_editor_hint_string += ",JetBrains Rider:5";
	external_editor_hint_string += ",JetBrains Fleet:7";
	external_editor_hint_string += ",Custom:6";
#endif
	EDITOR_DEF_BASIC("dotnet/editor/external_editor", 0);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, "dotnet/editor/external_editor", PROPERTY_HINT_ENUM, external_editor_hint_string));

	EDITOR_DEF_BASIC("dotnet/editor/custom_exec_path", "");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, "dotnet/editor/custom_exec_path", PROPERTY_HINT_GLOBAL_FILE));
	EDITOR_DEF_BASIC("dotnet/editor/custom_exec_path_args", "{file}");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, "dotnet/editor/custom_exec_path_args"));
}
#endif

void DotNetModule::register_project_settings() {
	GLOBAL_DEF("dotnet/project/assembly_name", "");

#ifdef TOOLS_ENABLED
	GLOBAL_DEF("dotnet/project/solution_directory", "");
#endif
}

#ifdef TOOLS_ENABLED
void DotNetModule::_start_fs_watcher() {
	fs_watcher->start();
}

void DotNetModule::_on_project_assembly_changed(FileSystemWatcher::FileSystemChange change_type) {
	DEV_ASSERT(runtime_manager != nullptr);

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	switch (change_type) {
		case FileSystemWatcher::FILE_SYSTEM_CHANGE_CREATE:
			if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to load assembly '" + assembly_name + "'.");
			}
			break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_DELETE:
			if (!runtime_manager->try_unload_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to unload assembly '" + assembly_name + "'.");
			}
			break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_MODIFY:
			if (!runtime_manager->try_reload_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to reload assembly '" + assembly_name + "'.");
			}
			break;
	}
}

void DotNetModule::_on_project_settings_changed() {
	if (init_state != InitState::INITIALIZED) {
		return;
	}

	bool dotnet_settings_changed = false;
	const PackedStringArray &changed_settings = ProjectSettings::get_singleton()->get_changed_settings();
	for (const String &setting : changed_settings) {
		if (setting.begins_with("dotnet/")) {
			dotnet_settings_changed = true;
			break;
		}
	}
	if (!dotnet_settings_changed) {
		// No .NET-related settings were changed, so we can ignore this.
		return;
	}

	const String assembly_name = Dirs::get_project_assembly_name();
	if (loaded_user_assembly_name == assembly_name) {
		// The assembly name didn't change, so no need to do anything.
		return;
	}

	change_project_assembly(assembly_name);
}

void DotNetModule::change_project_assembly(const String &p_assembly_name) {
	DEV_ASSERT(runtime_manager != nullptr);

	if (changing_project_assembly) {
		// The project assembly is already changing.
		return;
	}

	changing_project_assembly = true;

	const String old_assembly_name = loaded_user_assembly_name;
	const String old_assemblies_dir = Dirs::get_project_assemblies_path();

	// Unload the currently loaded assembly (if any).
	if (!old_assembly_name.is_empty()) {
		if (!runtime_manager->try_unload_extension(old_assembly_name, old_assemblies_dir)) {
			WARN_PRINT(".NET: Failed to unload assembly '" + old_assembly_name + "'.");
		}
	}

	// Update the project setting and persist it.
	ProjectSettings::get_singleton()->set("dotnet/project/assembly_name", p_assembly_name);
	ProjectSettings::get_singleton()->save();
	Dirs::invalidate_cached_directories();

	const String new_assembly_name = Dirs::get_project_assembly_name();
	const String new_assemblies_dir = Dirs::get_project_assemblies_path();
	DEV_ASSERT(new_assembly_name == p_assembly_name);

	// Update the file system watcher to watch the new assembly path.
	const String new_dll_path = new_assemblies_dir.path_join(new_assembly_name + ".dll");
	fs_watcher->set_path(new_dll_path);

	// Load the new assembly if it's already built, otherwise set the appropriate state.
	if (FileAccess::exists(new_dll_path)) {
		if (!runtime_manager->try_load_extension(new_assembly_name, new_assemblies_dir)) {
			WARN_PRINT(".NET: Failed to load assembly '" + new_assembly_name + "'.");
			OS::get_singleton()->alert(TTR("Failed to load assembly '%s'. Check the console output for more details.", new_assembly_name));
		}
	} else if (FileAccess::exists(Dirs::get_project_csproj_path())) {
		OS::get_singleton()->alert(TTR("Assembly '%s' not found. Please build the project to generate the assembly.", new_assembly_name));
	} else {
		OS::get_singleton()->alert(TTR("Project file '%s' not found. Please ensure there is a .csproj file in the project directory and build it to generate the assembly.", Dirs::get_project_csproj_path()));
	}

	changing_project_assembly = false;
}

bool DotNetModule::try_restore_editor_packages(const String &p_editor_assemblies_path) {
	const String proj_path = p_editor_assemblies_path.path_join("RestoreEditorPackages.proj");

	// Create 'RestoreEditorPackages.proj' that restores the packages.

	Error err = DirAccess::make_dir_recursive_absolute(p_editor_assemblies_path);
	ERR_FAIL_COND_V_MSG(err != OK, false, "Could not create directory '" + p_editor_assemblies_path + "'.");
	Ref<FileAccess> proj_file = FileAccess::open(proj_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err != OK, false, "Could not open '" + proj_path + "'.");
	proj_file->store_buffer((uint8_t *)_RestoreEditorPackages_proj, strlen(_RestoreEditorPackages_proj));
	proj_file->close();

	// Build 'RestoreEditorPackages.proj' to restore the packages.

	String pipe;
	List<String> args;
	args.push_back("build");
	args.push_back(proj_path);

	int exitcode;
	err = OS::get_singleton()->execute("dotnet", args, &pipe, &exitcode, true);

	ERR_FAIL_COND_V_MSG(false, err != OK, "Failed to restore editor packages. Error: " + String(error_names[err]));
	if (exitcode != 0) {
		// Print the process output because it may contain more details about the error.
		print_line(pipe);
		ERR_FAIL_V_MSG(false, "Failed to restore editor packages. See output above for more details.");
	}

	return true;
}
#endif

DotNetModule::DotNetModule() {
	singleton = this;
}

DotNetModule::~DotNetModule() {
	singleton = nullptr;

	if (runtime_manager != nullptr) {
		memdelete(runtime_manager);
	}
}

} // namespace DotNet
