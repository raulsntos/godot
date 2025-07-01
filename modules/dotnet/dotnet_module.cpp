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

#ifdef TOOLS_ENABLED
#include "editor/RestoreEditorPackages.proj.gen.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
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
	// TODO: The editor always needs to initialize the .NET module for now.
	return true;
#else
	return OS::get_singleton()->has_feature("dotnet");
#endif
}

void DotNetModule::initialize() {
	print_verbose(".NET: Initializing module.");

	runtime_manager = memnew(DotNetRuntimeManager);
	CRASH_COND(runtime_manager == nullptr);

#ifdef TOOLS_ENABLED
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	bool integration_loaded = try_restore_editor_packages(editor_assemblies_dir) && runtime_manager->try_load_extension("Godot.EditorIntegration", editor_assemblies_dir);
	if (!integration_loaded) {
		// If the editor integration could not be loaded, .NET-related functionality can't be enabled
		// (i.e.: .NET assemblies can't be loaded). Show an message to users to try to explain and crash
		// because we can't recover from this error.
		OS::get_singleton()->alert(TTR("Failed to load .NET editor integration. Check the console output for more details."), TTR("Failed to load .NET editor integration"));
		CRASH_NOW_MSG(".NET: Failed to load editor integration.");
	}
#endif

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	bool should_load_project_assembly = true;

#ifdef TOOLS_ENABLED
	fs_watcher.instantiate();
	fs_watcher->path = assemblies_dir.path_join(assembly_name + ".dll");
	fs_watcher->callable = callable_mp(this, &DotNetModule::on_project_assembly_changed).bind(assembly_name, assemblies_dir);

	should_load_project_assembly = FileAccess::exists(fs_watcher->path);
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
void DotNetModule::on_project_assembly_changed(uint64_t change_type) {
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
