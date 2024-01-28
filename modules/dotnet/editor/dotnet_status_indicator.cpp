/**************************************************************************/
/*  dotnet_status_indicator.cpp                                           */
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

#include "dotnet_status_indicator.h"

#include "../dotnet_module.h"
#include "dotnet_status_panel.h"

#include "core/io/dir_access.h"
#include "core/object/callable_mp.h"
#include "scene/gui/button.h"

namespace DotNet {

DotNetStatusIndicator *DotNetStatusIndicator::singleton = nullptr;

DotNetStatusIndicator *DotNetStatusIndicator::get_singleton() {
	return singleton;
}

void DotNetStatusIndicator::_update_indicator() {
	const DotNetModule *module = DotNetModule::get_singleton();
	if (module == nullptr) {
		hide();
		return;
	}

	const DotNetModuleState *state = module->get_state();
	DEV_ASSERT(state != nullptr);

	DotNetModuleState::InitState init_state = state->get_initialization_state();

	bool is_hidden = false;
	bool is_disabled = false;
	bool is_module_enabled = false;
	String tooltip;
	String panel_title;
	String panel_message;
	severity = Severity::NONE;

	switch (init_state) {
		case DotNetModuleState::InitState::UNINITIALIZED: {
			// When the module is not initialized, we only want to show the indicator if there is a C# project.
			// Otherwise, we hide it because it has no relevance to users that don't use C#.
			panel_title = TTR(".NET features disabled");
			tooltip = TTR(".NET features available.\nClick for details.");
			if (_project_has_csproj_files()) {
				severity = Severity::WARNING;
				panel_message = TTR("Found C# project in the workspace.");
			} else {
				is_hidden = true;
			}
		} break;

		case DotNetModuleState::InitState::INITIALIZING: {
			// During initialization, the button is disabled. Unless it's taking too long.
			is_disabled = true;
			tooltip = TTR(".NET features initializing...");
			panel_title = TTR(".NET features initializing...");
			panel_message = TTR(".NET features are initializing. Please wait...");
			severity = Severity::LOADING;
			if (state->is_initialization_timed_out()) {
				is_disabled = false;
				severity = Severity::WARNING;
				tooltip = TTR(".NET initialization is taking longer than expected.\nClick for details.");
				panel_message = TTR(".NET initialization is taking longer than expected. Please check the console output for more details. If the problem persists, restart the editor.");
			}
		} break;

		case DotNetModuleState::InitState::FAILED: {
			severity = Severity::ERROR;
			panel_title = TTR(".NET features disabled");
			tooltip = TTR(".NET module failed to initialize.\nClick for details.");
			switch (state->get_initialization_failed_state()) {
				case DotNetModuleState::ModuleFailedState::DOTNET_SDK_NOT_FOUND:
					panel_message = TTR("The .NET SDK was not found.");
					break;
				case DotNetModuleState::ModuleFailedState::EDITOR_INTEGRATION_RESTORE_FAILED:
					panel_message = TTR("Failed to download the .NET editor integration.");
					break;
				case DotNetModuleState::ModuleFailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE:
					panel_message = TTR("Failed to load the .NET editor integration.");
					break;
				default:
					panel_message = TTR("An unknown error occurred during .NET initialization. Please check the console output for more details.");
					break;
			}
		} break;

		case DotNetModuleState::InitState::INITIALIZED: {
			is_module_enabled = true;
			panel_title = TTR(".NET features enabled");
			tooltip = TTR(".NET features enabled.\nClick for details.");
		} break;
	}

	if (is_hidden) {
		hide();
		return;
	}

	show();

	main_button->set_tooltip_text(tooltip);
	main_button->set_disabled(is_disabled);
	main_button->set_button_icon(get_editor_theme_icon(is_module_enabled ? SNAME("CSharpStatusIndicator") : SNAME("CSharpStatusIndicatorDisabled")));

	status_panel->set_title(panel_title);
	status_panel->set_message(panel_message);
	status_panel->set_initial_severity(severity);
	status_panel->update();

	// The severity may be updated in the status panel update callback,
	// so we need to redraw the button at the end.
	main_button->queue_redraw();
}

void DotNetStatusIndicator::_update_panel_position() {
	Size2 panel_size = status_panel->get_contents_minimum_size();
	Point2 panel_position = get_screen_position() - Point2(0, panel_size.y);
	if (!is_layout_rtl()) {
		panel_position.x = panel_position.x - panel_size.x + get_size().x;
	}

	status_panel->set_position(panel_position);
	status_panel->reset_size();
}

void DotNetStatusIndicator::_show_status_panel() {
	_update_panel_position();
	status_panel->popup();
}

void DotNetStatusIndicator::_hide_status_panel() {
	status_panel->hide();
}

void DotNetStatusIndicator::toggle_status_panel() {
	if (status_panel->is_visible()) {
		_hide_status_panel();
	} else {
		_show_status_panel();
	}
}

void DotNetStatusIndicator::_draw_button() {
	if (!is_visible()) {
		return;
	}

	// We may draw an additional icon overlay on the button to indicate there's something
	// that needs attention. Like a C# project was found but the module is not initialized,
	// the module failed to initialize, or the loaded project has a problem.
	Ref<Texture2D> overlay_icon;
	switch (severity) {
		case Severity::LOADING:
			overlay_icon = get_editor_theme_icon(SNAME("Progress1"));
			break;
		case Severity::WARNING:
			overlay_icon = get_editor_theme_icon(SNAME("StatusWarning"));
			break;
		case Severity::ERROR:
			overlay_icon = get_editor_theme_icon(SNAME("StatusError"));
			break;
		default:
			break;
	}
	if (overlay_icon.is_null()) {
		return;
	}

	real_t button_radius = main_button->get_size().y / 8;
	Vector2 dot_position = Vector2(button_radius * 2, button_radius * 2);
	main_button->draw_texture(overlay_icon, dot_position - overlay_icon->get_size() / 2);
}

bool _project_has_csproj_files_core(const String &p_root_path) {
	Ref<DirAccess> dir_access = DirAccess::open(p_root_path);
	if (dir_access.is_null()) {
		return false;
	}

	for (const String &filename : dir_access->get_files()) {
		if (filename.has_extension("csproj")) {
			return true;
		}
	}

	for (const String &subdir : dir_access->get_directories()) {
		if (subdir == "." || subdir == "..") {
			continue;
		}

		if (subdir.begins_with(".")) {
			// Also skip hidden directories (like '.godot' and '.git') which are unlikely
			// to contain .csproj files that the user wants to use.
			continue;
		}

		if (_project_has_csproj_files_core(p_root_path.path_join(subdir))) {
			return true;
		}
	}

	return false;
}

bool DotNetStatusIndicator::_project_has_csproj_files() {
	const String &root_path = "res://";
	return _project_has_csproj_files_core(root_path);
}

void DotNetStatusIndicator::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_indicator();
		} break;
	}
}

void DotNetStatusIndicator::set_severity(Severity p_severity) {
	if (!updating) {
		// This method is meant to be called from the status panel update callback,
		// otherwise it will not redraw the button.
		WARN_PRINT("DotNetStatusIndicator::set_severity should only be called from the status panel update callback to ensure the button is redrawn.");
		return;
	}

	severity = p_severity;
}

void DotNetStatusIndicator::set_panel_content(Control *p_content) {
	status_panel->set_content(p_content);
}

void DotNetStatusIndicator::update() {
	updating = true;
	_update_indicator();
	updating = false;
}

DotNetStatusIndicator::DotNetStatusIndicator() {
	singleton = this;

	hide();

	// Main button.
	main_button = memnew(Button);
	main_button->set_theme_type_variation("FlatMenuButton");
	main_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusIndicator::toggle_status_panel));
	main_button->connect(SceneStringName(draw), callable_mp(this, &DotNetStatusIndicator::_draw_button));
	add_child(main_button);

	// Status panel.
	status_panel = memnew(DotNetStatusPanel);
	add_child(status_panel);
}

DotNetStatusIndicator::~DotNetStatusIndicator() {
	singleton = nullptr;
}

} // namespace DotNet
