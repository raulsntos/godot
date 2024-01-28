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
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/tween.h"
#include "scene/gui/button.h"

namespace DotNet {

DotNetStatusIndicator *DotNetStatusIndicator::singleton = nullptr;

DotNetStatusIndicator *DotNetStatusIndicator::get_singleton() {
	return singleton;
}

Ref<Tween> DotNetStatusIndicator::_create_shake_tween() {
	Ref<Tween> tween = create_tween();

	main_button->set_offset_transform_enabled(true);
	real_t rotation_target = 0.04f * Math::TAU;

	Ref<Tween> main_tween = create_tween();
	Ref<Tween> rotate_tween = create_tween();
	Ref<Tween> scale_tween = create_tween();

	const NodePath &rotation_property = NodePath("offset_transform_rotation");
	const NodePath &scale_property = NodePath("offset_transform_scale");

	// 0%
	rotate_tween->tween_property(main_button, rotation_property, 0.0, 0.0);
	scale_tween->tween_property(main_button, scale_property, Vector2(1, 1), 0.0);

	// 15%
	rotate_tween->tween_property(main_button, rotation_property, rotation_target, 0.12);
	scale_tween->tween_property(main_button, scale_property, Vector2(1.1, 1.1), 0.12);

	// 30%
	rotate_tween->tween_property(main_button, rotation_property, -rotation_target, 0.12);
	scale_tween->tween_property(main_button, scale_property, Vector2(1.2, 1.2), 0.12);

	// 45%
	rotate_tween->tween_property(main_button, rotation_property, rotation_target, 0.12);
	scale_tween->tween_property(main_button, scale_property, Vector2(1.1, 1.1), 0.12);

	// 60%
	rotate_tween->tween_property(main_button, rotation_property, -rotation_target, 0.12);
	scale_tween->tween_property(main_button, scale_property, Vector2(1.2, 1.2), 0.12);

	// 100%
	rotate_tween->tween_property(main_button, rotation_property, 0.0, 0.32);
	scale_tween->tween_property(main_button, scale_property, Vector2(1, 1), 0.32);

	main_tween->parallel()->tween_subtween(rotate_tween);
	main_tween->parallel()->tween_subtween(scale_tween);

	// Add a 300ms delay to give time for the animation to be noticeable,
	// especially when it triggers during editor initialization.
	tween->tween_interval(0.3);
	tween->tween_subtween(main_tween);

	return tween;
}

void DotNetStatusIndicator::_update_indicator() {
	const DotNetModule *module = DotNetModule::get_singleton();
	if (module == nullptr) {
		hide();
		return;
	}

	const DotNetModuleState *state = module->get_state();
	DEV_ASSERT(state != nullptr);

	InitState init_state = state->get_initialization_state();

	Severity old_severity = severity;

	bool is_hidden = false;
	bool is_disabled = false;
	bool is_module_enabled = false;
	String tooltip;
	String panel_title;
	String panel_message;
	severity = Severity::NONE;

	switch (init_state) {
		case InitState::UNINITIALIZED: {
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

		case InitState::INITIALIZING: {
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

		case InitState::FAILED: {
			severity = Severity::ERROR;
			panel_title = TTR(".NET features disabled");
			tooltip = TTR(".NET module failed to initialize.\nClick for details.");
			switch (state->get_initialization_failed_state()) {
				case ModuleFailedState::DOTNET_SDK_NOT_FOUND:
					panel_message = TTR("The .NET SDK was not found.");
					break;
				case ModuleFailedState::EDITOR_INTEGRATION_RESTORE_FAILED:
					panel_message = TTR("Failed to download the .NET editor integration.");
					break;
				case ModuleFailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE:
					panel_message = TTR("Failed to load the .NET editor integration.");
					break;
				default:
					panel_message = TTR("An unknown error occurred during .NET initialization. Please check the console output for more details.");
					break;
			}
		} break;

		case InitState::INITIALIZED: {
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

	// If the severity changed to a level that requires user attention, we play a shake animation.
	// If the severity didn't change, we don't want to kill the running animation.
	if (severity != old_severity) {
		shake_animation_queued = true;
	}
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

void DotNetStatusIndicator::_process(double p_delta) {
	// If the editor is ready, we can start playing the shake animation if it was queued.
	// Otherwise, we'll keep waiting until it's ready to make sure it's visible when it plays.
	if (EditorNode::get_singleton()->is_editor_ready()) {
		// We have to make sure the severity is still at a level that requires attention
		// because it may have changed since the animation was queued.
		if (shake_animation_queued && (severity == Severity::ERROR || severity == Severity::WARNING)) {
			shake_animation_queued = false;
			if (shake_tween.is_valid()) {
				shake_tween->kill();
			}
			shake_tween = _create_shake_tween();
		}
	}

	// The button is only animated when it's in the LOADING state and visible.
	if (!is_visible() || severity != Severity::LOADING) {
		return;
	}

	elapsed_time += p_delta;
	main_button->queue_redraw();
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
	Color color;
	switch (severity) {
		case Severity::LOADING:
			color = get_theme_color(SNAME("mono_color"), EditorStringName(Editor));
			break;
		case Severity::WARNING:
			color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
			break;
		case Severity::ERROR:
			color = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
			break;
		default:
			return;
	}

	real_t button_radius = 3.5 * EDSCALE;
	Vector2 dot_position = Vector2(button_radius * 2, button_radius * 2);

	// First, we draw a background circle where the dot will be to "clear" the area
	// so the dot doesn't overlap with the button icon.
	Color background_color = get_theme_color(SNAME("dark_color_1"), EditorStringName(Editor));
	main_button->draw_circle(dot_position, button_radius + 2, background_color);

	// Then, we draw the actual dot. For loading, we draw an animated arc that looks like a spinner instead.
	if (severity == Severity::LOADING) {
		real_t rotation_speed = 4.0;

		real_t spinner_angle = Math::fmod(elapsed_time * rotation_speed, Math::TAU);
		real_t wave = Math::abs(Math::sin(elapsed_time * rotation_speed / 2.0));

		real_t min_length = 0.4;
		real_t max_length = Math::PI;
		real_t spinner_length = min_length + (wave * (max_length - min_length));

		real_t start_angle = spinner_angle;
		real_t end_angle = spinner_angle + spinner_length;
		main_button->draw_arc(dot_position, button_radius, start_angle, end_angle, 8, color, 2.0);
	} else {
		main_button->draw_circle(dot_position, button_radius, color);
	}
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

		case NOTIFICATION_PROCESS: {
			if (is_visible()) {
				double delta = get_process_delta_time();
				_process(delta);
			}
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

	// Enable the process notification.
	set_process(true);
	set_process_mode(PROCESS_MODE_ALWAYS);

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
