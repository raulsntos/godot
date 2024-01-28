/**************************************************************************/
/*  dotnet_module_state.cpp                                               */
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

#include "dotnet_module_state.h"

#include "core/object/callable_mp.h"
#include "scene/main/scene_tree.h"

#ifdef TOOLS_ENABLED
#include "./editor/dotnet_status_indicator.h"
#endif

namespace DotNet {

#ifdef TOOLS_ENABLED
void DotNetModuleState::_start_init_timer() {
	Ref<SceneTreeTimer> timer = SceneTree::get_singleton()->create_timer(5.0);
	timer->connect(SNAME("timeout"), callable_mp(this, &DotNetModuleState::_on_init_timeout));
}

void DotNetModuleState::_on_init_timeout() {
	if (init_state != InitState::INITIALIZING) {
		return;
	}

	init_timed_out = true;
	notify_state_changed();
}
#endif

void DotNetModuleState::_update_module_initialization_state(InitState p_init_state) {
	init_state = p_init_state;

#ifdef TOOLS_ENABLED
	init_timed_out = false;

	if (p_init_state == InitState::INITIALIZING) {
		// Start a timer to check if initialization is taking too long.
		callable_mp(this, &DotNetModuleState::_start_init_timer).call_deferred();
	}

	notify_state_changed();
#endif
}

void DotNetModuleState::_update_assembly_load_state(InitState p_init_state) {
	assembly_load_state = p_init_state;

#ifdef TOOLS_ENABLED
	notify_state_changed();
#endif
}

#ifdef TOOLS_ENABLED
void DotNetModuleState::notify_state_changed() {
	DotNetStatusIndicator *status_indicator = DotNetStatusIndicator::get_singleton();
	if (status_indicator != nullptr) {
		// We may receive this notification at any time,
		// but the update can only be called in the main thread,
		// so we call it deferred to synchronize with the main thread.
		callable_mp(status_indicator, &DotNetStatusIndicator::update).call_deferred();
	}
}
#endif

void _print_err(const String &p_error) {
	ERR_PRINT_ED(vformat(".NET: %s", p_error));
}

void DotNetModuleState::start_module_initialization() {
	failed_state = ModuleFailedState::NONE;
	_update_module_initialization_state(InitState::INITIALIZING);
}

void DotNetModuleState::complete_module_initialization() {
	failed_state = ModuleFailedState::NONE;
	_update_module_initialization_state(InitState::INITIALIZED);
}

void DotNetModuleState::fail_module_initialization(ModuleFailedState p_failed_state, const String &p_error) {
	failed_state = p_failed_state;
	_update_module_initialization_state(InitState::FAILED);

	// Print the error message deferred so the editor toaster is ready to show it,
	// otherwise it would only show in the console.
	callable_mp_static(_print_err).call_deferred(p_error);
}

void DotNetModuleState::start_assembly_load() {
#ifdef TOOLS_ENABLED
	failed_assembly_state = AssemblyLoadFailedState::NONE;
	current_assembly_name = "";
#endif
	_update_assembly_load_state(InitState::INITIALIZING);
}

void DotNetModuleState::complete_assembly_load(const String &p_assembly_name) {
#ifdef TOOLS_ENABLED
	failed_assembly_state = AssemblyLoadFailedState::NONE;
	current_assembly_name = p_assembly_name;
#endif
	_update_assembly_load_state(InitState::INITIALIZED);
}

#ifdef TOOLS_ENABLED
void DotNetModuleState::fail_assembly_load(AssemblyLoadFailedState p_failed_state) {
	failed_assembly_state = p_failed_state;
	current_assembly_name = "";
#else
void DotNetModuleState::fail_assembly_load() {
#endif
	_update_assembly_load_state(InitState::FAILED);
}

} // namespace DotNet
