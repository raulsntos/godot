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

// #include "welcome_dialog.h"

#include "../dotnet_module.h"

#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/texture_rect.h"

namespace DotNet {

void DotNetStatusPanel::_initialize_module() {
	hide();

	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);
	if (module->get_init_state() == DotNetModule::InitState::FAILED) {
		// If previously failed, retry initialization.
		module->initialize();
		return;
	}

	// WelcomeDialog *welcome_dialog = WelcomeDialog::get_singleton();
	// DEV_ASSERT(welcome_dialog != nullptr);
	// welcome_dialog->popup_centered();
	module->initialize();
}

void DotNetStatusPanel::_select_project() {
	hide();

	// TODO(@raulsntos): The quick open dialog only works with Resources but ".csproj" aren't, we also only support the ones in the root directory which I don't think can be restricted by this dialog, so we may have to create our own dialog for this.
	Vector<StringName> base_types;
	base_types.append("csproj");

	EditorQuickOpenDialog *quick_open = EditorNode::get_singleton()->get_quick_open_dialog();
	quick_open->popup_dialog(base_types, callable_mp(this, &DotNetStatusPanel::_load_project));
}

void DotNetStatusPanel::_load_project(const String &p_path) {
	// TODO(@raulsntos): We should globalize the path, then use `dotnet --getProperty:OutputPath` to get the DLL path and load the assembly (see how we do it in `DotNetModule`, but make sure to unload the current project first if there's any). Even if the same project as the currently loaded project is selected, we should still do this, it serves as a manual reload.
	print_line(vformat("Selected project: %s"), p_path);
}

void DotNetStatusPanel::_open_url(const String &p_url) {
	hide();

	OS::get_singleton()->shell_open(p_url);
}

void DotNetStatusPanel::_copy_to_clipboard(const String &p_text) {
	hide();

	DisplayServer::get_singleton()->clipboard_set(p_text);
}

// TODO(@raulsntos): It may be better to reuse nodes instead of clearing and recreating them every time.
void clear_children(Node *p_node) {
	for (int i = p_node->get_child_count(); i > 0; i--) {
		Node *child = p_node->get_child(i - 1);
		p_node->remove_child(child);
		child->queue_free();
	}
}

void DotNetStatusPanel::_update_content() {
	// TODO(@raulsntos): This should never happen, and it's basically the same as FAILED except it doesn't allow you to retry. We should see if we can somehow avoid this state.
	DotNetModule *module = DotNetModule::get_singleton();
	if (module == nullptr) {
		status_label->set_text(TTR("Module not available"));
		clear_children(details);
		enable_button->hide();
		return;
	}

	DotNetModule::InitState init_state = module->get_init_state();

	switch (init_state) {
		case DotNetModule::InitState::DISABLED:
		case DotNetModule::InitState::INITIALIZING: {
			status_label->set_text(TTR(".NET features disabled"));

			clear_children(details);
			if (module->project_has_csproj_files()) {
				HBoxContainer *hbox = memnew(HBoxContainer);

				TextureRect *icon = memnew(TextureRect);
				icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
				icon->set_texture(get_theme_icon(SNAME("NodeWarning"), EditorStringName(EditorIcons)));
				hbox->add_child(icon);

				Label *label = memnew(Label);
				label->set_text(TTR("Found C# files in the project."));
				hbox->add_child(label);

				details->add_child(hbox);
			} else {
				Label *label = memnew(Label);
				label->set_text(TTR("No C# files found in the project."));
				details->add_child(label);
			}

			enable_button->set_text(TTR("Enable .NET features"));
			enable_button->show();
		} break;
		case DotNetModule::InitState::INITIALIZED: {
			status_label->set_text(TTR(".NET features enabled"));

			// TODO(@raulsntos): Add links to documentation about using C# in Godot? Or maybe just a button to open the Welcome Dialog again?
			clear_children(details);

			// Godot.EditorIntegration line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				Label *version_label = memnew(Label);
				version_label->set_text(TTR("Godot .NET editor integration"));
				hbox->add_child(version_label);

				hbox->add_spacer();

				const String &editor_integration_version = module->get_editor_integration_version();

				LinkButton *version_info_button = memnew(LinkButton);
				version_info_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				version_info_button->set_text(editor_integration_version);
				version_info_button->set_tooltip_text(TTR("Click to copy the version information."));
				version_info_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_copy_to_clipboard).bind(vformat("Godot.EditorIntegration %s", editor_integration_version)));
				hbox->add_child(version_info_button);

				details->add_child(hbox);
			}

			// .NET SDK line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				Label *sdk_label = memnew(Label);
				sdk_label->set_text(TTR(".NET SDK"));
				hbox->add_child(sdk_label);

				hbox->add_spacer();

				const String &dotnet_sdk_version = module->get_dotnet_sdk_version();
				const String &dotnet_sdk_path = module->get_dotnet_sdk_path();

				LinkButton *sdk_info_button = memnew(LinkButton);
				sdk_info_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				sdk_info_button->set_text(dotnet_sdk_version);
				sdk_info_button->set_tooltip_text(TTR("Click to copy the version information."));
				sdk_info_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_copy_to_clipboard).bind(vformat("%s [%s]", dotnet_sdk_version, dotnet_sdk_path)));
				hbox->add_child(sdk_info_button);

				details->add_child(hbox);
			}

			// Project line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				const String &assembly_name = module->get_loaded_user_assembly_name();

				Label *project_label = memnew(Label);
				if (assembly_name.is_empty()) {
					project_label->set_text(TTR("No C# project loaded."));
				} else {
					project_label->set_text(vformat("%s.csproj", assembly_name));
				}
				hbox->add_child(project_label);

				hbox->add_spacer();

				LinkButton *select_project_button = memnew(LinkButton);
				select_project_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				if (assembly_name.is_empty()) {
					select_project_button->set_text(TTR("Open"));
				} else {
					select_project_button->set_text(TTR("Change"));
				}
				select_project_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_select_project));
				// TODO(@raulsntos): We don't support changing projects yet, see `DotNetStatusPanel::_select_project`. For now we'll just hide the button.
				// hbox->add_child(select_project_button);

				details->add_child(hbox);
			}

			enable_button->hide();
		} break;
		case DotNetModule::InitState::FAILED: {
			status_label->set_text(TTR(".NET features disabled - Failed to initialize"));

			clear_children(details);

			HBoxContainer *hbox = memnew(HBoxContainer);
			details->add_child(hbox);

			TextureRect *icon = memnew(TextureRect);
			icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			icon->set_texture(get_theme_icon(SNAME("StatusError"), EditorStringName(EditorIcons)));
			hbox->add_child(icon);

			switch (module->get_failed_state()) {
				case DotNetModule::FailedState::DOTNET_SDK_NOT_FOUND: {
					Label *label = memnew(Label);
					label->set_text(TTR("The .NET SDK was not found."));
					hbox->add_child(label);

					hbox->add_spacer();

					LinkButton *download_button = memnew(LinkButton);
					download_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
					download_button->set_text(TTR("Download"));
					download_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_open_url).bind("https://get.dot.net/"));
					hbox->add_child(download_button);
				} break;
				case DotNetModule::FailedState::EDITOR_INTEGRATION_RESTORE_FAILED: {
					Label *label = memnew(Label);
					label->set_text(TTR("Downloading the .NET editor integration failed."));
					hbox->add_child(label);
				} break;
				case DotNetModule::FailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE: {
					Label *label = memnew(Label);
					label->set_text(TTR("Loading the .NET editor integration failed."));
					hbox->add_child(label);
				} break;
				default: {
					Label *label = memnew(Label);
					label->set_text(TTR("An unknown error occurred. There may be more details in the console output."));
					hbox->add_child(label);
				} break;
			}

			enable_button->set_text(TTR("Retry"));
			enable_button->show();
		} break;
	}
}

void DotNetStatusPanel::refresh() {
	_update_content();
}

DotNetStatusPanel::DotNetStatusPanel() {
	VBoxContainer *vbox = memnew(VBoxContainer);
	vbox->set_custom_minimum_size(Size2(350 * EDSCALE, 0));
	add_child(vbox);

	status_label = memnew(Label);
	status_label->set_theme_type_variation("HeaderSmall");
	vbox->add_child(status_label);

	details = memnew(VBoxContainer);
	vbox->add_child(details);

	HBoxContainer *button_container = memnew(HBoxContainer);
	button_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vbox->add_child(button_container);

	enable_button = memnew(Button);
	enable_button->set_text(TTR("Enable .NET features"));
	enable_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_initialize_module));
	button_container->add_child(enable_button);
}

} // namespace DotNet
