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

#include "editor/editor_string_names.h"
#include "scene/gui/button.h"

namespace DotNet {

DotNetStatusIndicator *DotNetStatusIndicator::singleton = nullptr;

DotNetStatusIndicator *DotNetStatusIndicator::get_singleton() {
	return singleton;
}

void DotNetStatusIndicator::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_update_indicator();
		} break;
	}
}

void DotNetStatusIndicator::_show_status_panel() {
	status_panel->refresh();
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

void DotNetStatusIndicator::_update_panel_position() {
	Size2 panel_size = status_panel->get_contents_minimum_size();
	Point2 panel_position = get_screen_position() - Point2(0, panel_size.y);
	if (!is_layout_rtl()) {
		panel_position.x = panel_position.x - panel_size.x + get_size().x;
	}

	status_panel->set_position(panel_position);
	status_panel->reset_size();
}

void DotNetStatusIndicator::_draw_button() {
	if (!is_visible()) {
		return;
	}

	DotNetModule *module = DotNetModule::get_singleton();
	if (module == nullptr) {
		return;
	}

	DotNetModule::InitState init_state = module->get_init_state();

	if (init_state == DotNetModule::InitState::INITIALIZED) {
		return;
	}

	// For the non-initialized states, we may draw an additional icon to indicate there's somethin
	// that needs attention. Like a C# project was found but the module is not initialized, or the
	// module failed to initialize.

	Ref<Texture2D> icon;
	switch (init_state) {
		case DotNetModule::InitState::DISABLED:
			if (module->project_has_csproj_files()) {
				icon = get_theme_icon(SNAME("StatusWarning"), EditorStringName(EditorIcons));
			} else {
				return;
			}
			break;
		case DotNetModule::InitState::FAILED:
			icon = get_theme_icon(SNAME("StatusError"), EditorStringName(EditorIcons));
			break;
		default:
			return;
	}

	real_t button_radius = main_button->get_size().y / 8;
	Vector2 dot_position = Vector2(button_radius * 2, button_radius * 2);
	main_button->draw_texture(icon, dot_position - icon->get_size() / 2);
}

void DotNetStatusIndicator::_update_indicator() {
	DotNetModule *module = DotNetModule::get_singleton();
	if (module == nullptr) {
		hide();
		return;
	}

	DotNetModule::InitState init_state = module->get_init_state();

	// When the module is not initialized, we only want to show the indicator if there is a C# project,
	// or if initialization failed. Otherwise, we don't want to distract non-C# users with an indicator
	// that has no relevance to them.
	if (init_state == DotNetModule::InitState::DISABLED) {
		if (!module->project_has_csproj_files()) {
			hide();
			return;
		}
	}

	if (init_state == DotNetModule::InitState::INITIALIZED) {
		main_button->set_button_icon(get_editor_theme_icon(SNAME("CSharpStatusIndicator")));
	} else {
		main_button->set_button_icon(get_editor_theme_icon(SNAME("CSharpStatusIndicatorDisabled")));
	}

	if (init_state == DotNetModule::InitState::INITIALIZING) {
		main_button->set_disabled(true);
	} else {
		main_button->set_disabled(false);
	}

	show();

	switch (init_state) {
		case DotNetModule::InitState::DISABLED:
		case DotNetModule::InitState::INITIALIZING: {
			main_button->set_tooltip_text(TTR(".NET features available.\nClick for details."));
		} break;
		case DotNetModule::InitState::INITIALIZED: {
			main_button->set_tooltip_text(TTR(".NET features enabled.\nClick for details."));

		} break;
		case DotNetModule::InitState::FAILED: {
			main_button->set_tooltip_text(TTR(".NET module failed to initialize.\nClick for details."));
		} break;
		default: {
		} break;
	}

	main_button->queue_redraw();
}

void DotNetStatusIndicator::update() {
	_update_indicator();
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
