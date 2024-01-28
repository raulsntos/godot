/**************************************************************************/
/*  dotnet_status_panel.cpp                                               */
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

#include "dotnet_status_panel.h"

#include "../dotnet_module.h"
#include "dotnet_status_indicator.h"

#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"
#include "servers/display/display_server.h"

namespace DotNet {

void DotNetStatusPanel::_initialize_module() {
	hide();

	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	const DotNetModuleState *state = module->get_state();
	DEV_ASSERT(state != nullptr);

	if (state->get_initialization_state() == InitState::FAILED) {
		// If previously failed, retry initialization.
		module->initialize();
		return;
	}

	module->initialize();
}

void DotNetStatusPanel::_update_content() {
	const DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	const DotNetModuleState *state = module->get_state();
	DEV_ASSERT(state != nullptr);

	if (state->get_initialization_state() == InitState::INITIALIZED) {
		_update_detailed_content();
	} else {
		_update_basic_content();
	}
}

void DotNetStatusPanel::_update_basic_content() {
	full_details->hide();
	basic_details->show();

	const DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	const DotNetModuleState *state = module->get_state();
	DEV_ASSERT(state != nullptr);

	// This method should only be called when the module is not initialized.
	DEV_ASSERT(state->get_initialization_state() != InitState::INITIALIZED);

	enable_dotnet_button->hide();

	switch (state->get_initialization_state()) {
		case InitState::UNINITIALIZED:
			enable_dotnet_button->set_text(TTR("Enable .NET features"));
			enable_dotnet_button->show();
			break;
		case InitState::FAILED:
			enable_dotnet_button->set_text(TTR("Retry"));
			enable_dotnet_button->show();
			break;
		default:
			break;
	}
}

void DotNetStatusPanel::_update_detailed_content() {
	basic_details->hide();
	full_details->show();

	// This should never happen if Godot.EditorIntegration was initialized correctly.
	ERR_FAIL_COND_MSG(content == nullptr, ".NET status panel has no content.");

	// The detailed content is implemented in C# and must have an 'update' method.
	ERR_FAIL_COND_MSG(!content->has_method("update"), ".NET status panel content has no 'update' method.");
	content->call("update");
}

void DotNetStatusPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			set_initial_severity(DotNetStatusIndicator::Severity::WARNING);
		} break;
	}
}

void DotNetStatusPanel::set_title(const String &p_title) {
	title_label->set_text(p_title);
}

void DotNetStatusPanel::set_message(const String &p_message) {
	info_label->set_text(p_message);
}

void DotNetStatusPanel::set_initial_severity(DotNetStatusIndicator::Severity p_severity) {
	severity = p_severity;
	info_icon->hide();

	Ref<Texture2D> severity_icon;
	switch (p_severity) {
		case DotNetStatusIndicator::Severity::WARNING:
			severity_icon = get_editor_theme_icon(SNAME("StatusWarning"));
			break;
		case DotNetStatusIndicator::Severity::ERROR:
			severity_icon = get_editor_theme_icon(SNAME("StatusError"));
			break;
		default:
			break;
	}
	if (severity_icon.is_null()) {
		return;
	}

	info_icon->set_texture(severity_icon);
	info_icon->show();
}

void DotNetStatusPanel::set_content(Control *p_content) {
	if (content != nullptr) {
		full_details->remove_child(content);
		content->queue_free();
		content = nullptr;
	}

	if (p_content != nullptr) {
		full_details->add_child(p_content);
		content = p_content;
	}
}

void DotNetStatusPanel::update() {
	_update_content();
}

DotNetStatusPanel::DotNetStatusPanel() {
	VBoxContainer *vbox = memnew(VBoxContainer);
	vbox->set_custom_minimum_size(Size2(350 * EDSCALE, 0));
	add_child(vbox);

	title_label = memnew(Label);
	title_label->set_theme_type_variation("HeaderLarge");
	vbox->add_child(title_label);

	VBoxContainer *details_container = memnew(VBoxContainer);
	vbox->add_child(details_container);

	basic_details = memnew(VBoxContainer);
	details_container->add_child(basic_details);

	full_details = memnew(VBoxContainer);
	full_details->hide();
	details_container->add_child(full_details);

	HBoxContainer *info_container = memnew(HBoxContainer);
	basic_details->add_child(info_container);

	info_icon = memnew(TextureRect);
	info_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	info_container->add_child(info_icon);

	info_label = memnew(Label);
	info_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	info_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	info_label->set_custom_minimum_size(Size2(512 * EDSCALE, 1));
	info_container->add_child(info_label);

	HBoxContainer *button_container = memnew(HBoxContainer);
	button_container->set_alignment(BoxContainer::ALIGNMENT_END);
	basic_details->add_child(button_container);

	enable_dotnet_button = memnew(Button);
	enable_dotnet_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_initialize_module));
	button_container->add_child(enable_dotnet_button);
}

} // namespace DotNet
