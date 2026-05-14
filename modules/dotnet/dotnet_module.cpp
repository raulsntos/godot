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
#ifdef DOTNET_WELCOME_DIALOG_ENABLED
#include "./editor/welcome_dialog.h"
#endif
#include "runtime/hostfxr/hostfxr_dotnet_runtime.h"

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"
#endif

namespace DotNet {

DotNetModule *DotNetModule::singleton = nullptr;

DotNetRuntimeManager *DotNetModule::runtime_manager = nullptr;

DotNetModule *DotNetModule::get_singleton() {
	return singleton;
}

bool DotNetModule::should_initialize() {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_project_manager_hint()) {
		// Never initialize the module in the project manager.
		return false;
	}

#ifdef DOTNET_WELCOME_DIALOG_ENABLED
	// If the editor packages are available, we can just initialize the module.
	// Otherwise, the welcome dialog will be displayed when the user attempts
	// to use any .NET features.
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	return DirAccess::exists(editor_assemblies_dir) && FileAccess::exists(editor_assemblies_dir.path_join("Godot.EditorIntegration.dll"));
#else
	return true;
#endif
#else
	// The exported project was built with .NET support,
	// so it needs to initialize the module.
	return OS::get_singleton()->has_feature("dotnet");
#endif
}

void DotNetModule::initialize() {
	print_verbose(".NET: Initializing module.");

	state->start_module_initialization();

	// When initializing for the first time, create the runtime manager.
	// If initialization fails and is retried, the runtime manager should already exist.
	if (runtime_manager == nullptr) {
		runtime_manager = memnew(DotNetRuntimeManager);
	}
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
		state->fail_module_initialization(ModuleFailedState::DOTNET_SDK_NOT_FOUND, TTR("Could not find .NET SDK. Please install from https://get.dot.net"));
		return;
	}

	print_verbose(vformat(".NET: .NET SDK installation found at '%s'.", found_dotnet_sdk_path));

	if (Engine::get_singleton()->is_editor_hint()) {
		const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
		if (!try_restore_editor_packages(editor_assemblies_dir)) {
			state->fail_module_initialization(ModuleFailedState::EDITOR_INTEGRATION_RESTORE_FAILED, TTR("Failed to restore .NET editor integration. Check the console output for more details."));
			return;
		}
		if (!runtime_manager->try_load_extension("Godot.EditorIntegration", editor_assemblies_dir)) {
			String error_message;
			switch (runtime_manager->get_last_failed_state()) {
				case AssemblyLoadFailedState::FAILED_TO_LOAD_HOSTFXR:
					error_message = TTR("Failed to initialize .NET runtime using hostfxr.");
					break;
				case AssemblyLoadFailedState::FAILED_TO_LOAD_PLUGIN_LOADER:
					error_message = TTR("Failed to initialize .NET assembly loader.");
					break;
				case AssemblyLoadFailedState::FAILED_TO_LOAD_GDEXTENSION_ENTRY_POINT:
					error_message = TTR("Failed to resolve entry point for the .NET editor integration.");
					break;
				case AssemblyLoadFailedState::FAILED_TO_LOAD_GDEXTENSION_INIT:
					error_message = TTR("Failed to initialize .NET editor integration.");
					break;
				default:
					error_message = TTR("Failed to load .NET editor integration.");
					break;
			}
			state->fail_module_initialization(ModuleFailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE, error_message);
			return;
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
	if (!should_load_project_assembly) {
		// DLL doesn't exist yet. Determine whether a .csproj is available.
		const String csproj_path = Dirs::get_project_csproj_path();
		if (FileAccess::exists(csproj_path)) {
			// The .csproj exists, so it was just not built yet.
			state->fail_assembly_load(AssemblyLoadFailedState::DLL_NOT_FOUND);
		} else {
			// There is no .csproj, so this project may not use C#.
			state->fail_assembly_load(AssemblyLoadFailedState::PROJECT_NOT_FOUND);
		}
	}
#endif

	if (should_load_project_assembly) {
		state->start_assembly_load();

		if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
#ifndef TOOLS_ENABLED
			// In the editor it's fine if the assembly fails to load, there are cases where this is expected
			// (e.g. the project hasn't been upgraded yet). The status indicator should take care of showing
			// users that the assembly is not loaded.
			ERR_PRINT(".NET: Failed to load assembly '" + assembly_name + "'.");
			state->fail_assembly_load();
#else
			state->fail_assembly_load(runtime_manager->get_last_failed_state());
#endif
		} else {
			state->complete_assembly_load(assembly_name);
		}
	}

	// In editor builds, the initialization has to wait for the editor integration which will
	// set the state to INITIALIZED once it's ready or FAILED if something goes wrong.
	// In exported builds, we can set the state to INITIALIZED right away since we don't have the editor integration.
#ifndef TOOLS_ENABLED
	state->complete_module_initialization();
#endif

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

	String unhandled_exception_policy_hint_string = "Log and continue:0";
	unhandled_exception_policy_hint_string += ",Terminate application:1";
	GLOBAL_DEF("dotnet/runtime/unhandled_exception_policy", 0);
	ProjectSettings::get_singleton()->set_custom_property_info(PropertyInfo(Variant::INT, "dotnet/runtime/unhandled_exception_policy", PROPERTY_HINT_ENUM, unhandled_exception_policy_hint_string));
}

#ifdef TOOLS_ENABLED
void DotNetModule::request_enable_dotnet_features() {
	// If the module is already initialized, .NET features are enabled.
	// If it's in the process of initializing, we should wait for it to succeed or fail.
	InitState init_state = state->get_initialization_state();
	if (init_state == InitState::INITIALIZING || init_state == InitState::INITIALIZED) {
		return;
	}

	// Otherwise, check if the editor packages are available.
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	if (DirAccess::exists(editor_assemblies_dir) && FileAccess::exists(editor_assemblies_dir.path_join("Godot.EditorIntegration.dll"))) {
		// The editor packages are available, so we can just initialize the module.
		initialize();
		return;
	}

	// The editor packages are not available, so we need to restore them.
	// We use the welcome dialog to introduce the .NET features to the user and ask for confirmation.
	// If we're in headless mode we can't show the dialog.
	if (!DisplayServer::get_singleton()->window_can_draw()) {
		// We're in headless mode, so we can't show the dialog.
		// The user may be running code in CI. Instead of hanging forever, we exit with an error code.
		SceneTree::get_singleton()->quit(-1);
		return;
	}

// If the welcome dialog is not available, this method was called too early.
// Wait until the editor has finished all initialization steps.
#ifdef DOTNET_WELCOME_DIALOG_ENABLED
	DotNet::WelcomeDialog *welcome_dialog = DotNet::WelcomeDialog::get_singleton();
	DEV_ASSERT(welcome_dialog != nullptr);
	welcome_dialog->popup_centered();
#else
	initialize();
#endif
}
#endif

#ifdef TOOLS_ENABLED
void DotNetModule::_start_fs_watcher() {
	fs_watcher->start();
}

void DotNetModule::_on_project_assembly_changed(FileSystemWatcher::FileSystemChange change_type) {
	DEV_ASSERT(runtime_manager != nullptr);

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	state->start_assembly_load();

	switch (change_type) {
		case FileSystemWatcher::FILE_SYSTEM_CHANGE_CREATE: {
			if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
				state->fail_assembly_load(runtime_manager->get_last_failed_state());
			} else {
				state->complete_assembly_load(assembly_name);
			}
		} break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_DELETE: {
			if (!runtime_manager->try_unload_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to unload assembly '" + assembly_name + "'.");
			}
			// The DLL was deleted. If the .csproj still exists the user probably cleaned
			// the build output; if it's gone too, there is no project available anymore.
			const String csproj_path = Dirs::get_project_csproj_path();
			if (FileAccess::exists(csproj_path)) {
				state->fail_assembly_load(AssemblyLoadFailedState::DLL_NOT_FOUND);
			} else {
				state->fail_assembly_load(AssemblyLoadFailedState::PROJECT_NOT_FOUND);
			}
		} break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_MODIFY: {
			// It's possible the assembly wasn't loaded even if the DLL existed,
			// because it could have failed to load earlier. So if it's not loaded,
			// we just load it and if it's loaded then we reload it.
			bool success;
			if (runtime_manager->is_assembly_loaded(assembly_name, assemblies_dir)) {
				success = runtime_manager->try_reload_extension(assembly_name, assemblies_dir);
			} else {
				success = runtime_manager->try_load_extension(assembly_name, assemblies_dir);
			}
			if (!success) {
				WARN_PRINT(".NET: Failed to reload assembly '" + assembly_name + "'.");
				state->fail_assembly_load(runtime_manager->get_last_failed_state());
			} else {
				state->complete_assembly_load(assembly_name);
			}
		} break;
	}
}

void DotNetModule::_on_project_settings_changed() {
	if (state->get_initialization_state() != InitState::INITIALIZED) {
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
	if (state->get_current_assembly_name() == assembly_name) {
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

	DotNetModuleState *state = get_state();

	const String old_assembly_name = state->get_current_assembly_name();
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

	state->start_assembly_load();

	// Load the new assembly if it's already built, otherwise set the appropriate state.
	if (FileAccess::exists(new_dll_path)) {
		if (!runtime_manager->try_load_extension(new_assembly_name, new_assemblies_dir)) {
			state->fail_assembly_load(runtime_manager->get_last_failed_state());
		} else {
			state->complete_assembly_load(new_assembly_name);
		}
	} else if (FileAccess::exists(Dirs::get_project_csproj_path())) {
		state->fail_assembly_load(AssemblyLoadFailedState::DLL_NOT_FOUND);
	} else {
		state->fail_assembly_load(AssemblyLoadFailedState::PROJECT_NOT_FOUND);
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

	ERR_FAIL_COND_V_MSG(err != OK, false, "Failed to restore editor packages. Error: " + String(error_names[err]));
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

	state = memnew(DotNetModuleState);
}

DotNetModule::~DotNetModule() {
	singleton = nullptr;

	if (runtime_manager != nullptr) {
		memdelete(runtime_manager);
		runtime_manager = nullptr;
	}

	memdelete(state);
}

} // namespace DotNet
