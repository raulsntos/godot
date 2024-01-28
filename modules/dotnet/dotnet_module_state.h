/**************************************************************************/
/*  dotnet_module_state.h                                                 */
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

#pragma once

#include "core/object/object.h"

namespace DotNet {

// Generic initialization state.
enum class InitState {
	UNINITIALIZED,
	INITIALIZING,
	INITIALIZED,
	FAILED,
};

// .NET module initialization failure reason, only meaningful when init_state == FAILED.
enum class ModuleFailedState {
	NONE,
	DOTNET_SDK_NOT_FOUND,
	EDITOR_INTEGRATION_RESTORE_FAILED,
	EDITOR_INTEGRATION_FAILED_TO_INITIALIZE,
};

#ifdef TOOLS_ENABLED
// User assembly load failure reason, only meaningful when assembly_load_state == FAILED.
enum class AssemblyLoadFailedState {
	NONE,
	PROJECT_NOT_FOUND,
	DLL_NOT_FOUND,
	FAILED_TO_LOAD,

	/* More specific FAILED_TO_LOAD reasons */

	/**
	 * Could not initialize the .NET runtime using hostfxr.
	 */
	FAILED_TO_LOAD_HOSTFXR,

	/**
	 * Could not initialize the 'Godot.PluginLoader' assembly.
	 */
	FAILED_TO_LOAD_PLUGIN_LOADER,

	/**
	 * Could not find the GDExtension entry point in the .NET assembly.
	 */
	FAILED_TO_LOAD_GDEXTENSION_ENTRY_POINT,

	/**
	 * When calling the GDExtension entry point, it returned false.
	 */
	FAILED_TO_LOAD_GDEXTENSION_INIT,
};
#endif

class DotNetModuleState : public Object {
private:
	// Module initialization state.
	InitState init_state = InitState::UNINITIALIZED;
	ModuleFailedState failed_state = ModuleFailedState::NONE;

#ifdef TOOLS_ENABLED
	bool init_timed_out = false;

	void _start_init_timer();
	void _on_init_timeout();
#endif

	// User assembly load state - Loading the DLL into the editor.
	InitState assembly_load_state = InitState::UNINITIALIZED;
#ifdef TOOLS_ENABLED
	String current_assembly_name;
	AssemblyLoadFailedState failed_assembly_state = AssemblyLoadFailedState::NONE;
#endif

	void _update_module_initialization_state(InitState p_init_state);
	void _update_assembly_load_state(InitState p_init_state);

public:
#ifdef TOOLS_ENABLED
	void notify_state_changed();
#endif

	InitState get_initialization_state() const { return init_state; }
#ifdef TOOLS_ENABLED
	bool is_initialization_timed_out() const { return init_state == InitState::INITIALIZING && init_timed_out; }
	ModuleFailedState get_initialization_failed_state() const { return failed_state; }
#endif

#ifdef TOOLS_ENABLED
	const String get_current_assembly_name() const { return current_assembly_name; }
	void get_assembly_load_state(String &r_assembly_name, InitState &r_init_state, AssemblyLoadFailedState &r_failed_state) const {
		r_assembly_name = current_assembly_name;
		r_init_state = assembly_load_state;
		r_failed_state = failed_assembly_state;
	}
#endif

	void start_module_initialization();
	void complete_module_initialization();
	void fail_module_initialization(ModuleFailedState p_failed_state, const String &p_error);

	void start_assembly_load();
	void complete_assembly_load(const String &p_assembly_name);
#ifdef TOOLS_ENABLED
	void fail_assembly_load(AssemblyLoadFailedState p_failed_state);
#else
	void fail_assembly_load();
#endif
};

} // namespace DotNet
