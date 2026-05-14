/**************************************************************************/
/*  welcome_dialog.cpp                                                    */
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

#ifdef DOTNET_WELCOME_DIALOG_ENABLED
#include "welcome_dialog.h"

#include "../dotnet_module.h"
#include "../runtime/hostfxr/hostfxr_dotnet_runtime.h"
#include "../utils/dirs_utils.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/margin_container.h"

namespace DotNet {

bool WelcomeDialog::QuickOptions::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;
	if (!section.is_empty()) {
		name = section + "/" + p_name;
	}

	if (!properties.has(name)) {
		return false;
	}

	bool ok;
	EditorSettings::get_singleton()->set(name, p_value, &ok);
	return ok;
}

bool WelcomeDialog::QuickOptions::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;
	if (!section.is_empty()) {
		name = section + "/" + p_name;
	}

	if (!properties.has(name)) {
		return false;
	}

	bool ok;
	r_ret = EditorSettings::get_singleton()->get(name, &ok);
	return ok;
}

void WelcomeDialog::QuickOptions::_get_property_list(List<PropertyInfo> *p_list) const {
	List<PropertyInfo> property_list;
	EditorSettings::get_singleton()->get_property_list(&property_list);

	for (PropertyInfo &property : property_list) {
		if (properties.has(property.name)) {
			if (property.name.begins_with(section + "/")) {
				property.name = property.name.replace_first(section + "/", "");
			}
			p_list->push_back(property);
		}
	}
}

bool WelcomeDialog::QuickOptions::_property_can_revert(const StringName &p_name) const {
	String name = p_name;
	if (!section.is_empty()) {
		name = section + "/" + p_name;
	}

	if (!properties.has(name)) {
		return false;
	}

	return EditorSettings::get_singleton()->property_can_revert(name);
}

bool WelcomeDialog::QuickOptions::_property_get_revert(const StringName &p_name, Variant &r_property) const {
	String name = p_name;
	if (!section.is_empty()) {
		name = section + "/" + p_name;
	}

	if (!properties.has(name)) {
		return false;
	}

	r_property = EditorSettings::get_singleton()->property_get_revert(name);
	return true;
}

void WelcomeDialog::QuickOptions::set_section(const String &p_section) {
	section = p_section;
}

void WelcomeDialog::QuickOptions::set_properties(PackedStringArray p_properties) {
	properties = p_properties;
}

Size2 WelcomeDialog::ButtonContainer::get_minimum_size() const {
	return container->get_minimum_size();
}

WelcomeDialog::ButtonContainer::ButtonContainer(Container *p_container) {
	container = p_container;
	add_child(container, false, INTERNAL_MODE_FRONT);
}

void WelcomeDialog::ButtonContainer::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_RESIZED:
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_VISIBILITY_CHANGED:
			container->set_size(get_size());
			break;
	}
}

WelcomeDialog *WelcomeDialog::singleton = nullptr;

WelcomeDialog *WelcomeDialog::get_singleton() {
	return singleton;
}
void WelcomeDialog::_check_dotnet_sdk() {
	String dotnet_sdk_path;
	bool dotnet_sdk_found = HostFxrDotNetRuntime::try_find_dotnet_sdk(dotnet_sdk_path);

	dotnet_sdk_prerequisite.fulfilled = dotnet_sdk_found;

	if (dotnet_sdk_found) {
		dotnet_sdk_prerequisite.title = TTR(".NET SDK ready");
		dotnet_sdk_prerequisite.status_message = vformat(TTR(".NET SDK installation found at '%s'."), dotnet_sdk_path);
		download_dotnet_sdk_button->hide();
	} else {
		dotnet_sdk_prerequisite.title = TTR(".NET SDK not found");
		dotnet_sdk_prerequisite.status_message = TTR("We could not find the .NET SDK or it's not properly configured.\nMake sure the dotnet CLI is available in your PATH environment variable.");
		download_dotnet_sdk_button->show();
	}

	_update();
}

void WelcomeDialog::_check_editor_integration() {
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	const String editor_integration_dll_path = editor_assemblies_dir.path_join("Godot.EditorIntegration.dll");

	editor_integration_progress_bar->hide();

	editor_integration_prerequisite.fulfilled = FileAccess::exists(editor_integration_dll_path) ? true : false;
	if (editor_integration_prerequisite.fulfilled) {
		editor_integration_prerequisite.title = TTR("C# editor integration ready");
		editor_integration_prerequisite.status_message = "";
		editor_integration_download_button->hide();
	} else {
		editor_integration_prerequisite.title = TTR("C# editor integration not found");
		editor_integration_prerequisite.status_message = TTR("Godot needs to download additional data to enable C# features in the editor.");
		editor_integration_download_button->set_text(TTR("Download and install C# editor integration"));
		editor_integration_download_button->show();
	}

	_update();
}

void WelcomeDialog::_update_prerequisite(PreRequisiteState state, PreRequisiteControls controls) {
	if (state.fulfilled) {
		controls.status_icon->set_texture(get_editor_theme_icon("StatusSuccess"));
	} else {
		controls.status_icon->set_texture(get_editor_theme_icon("StatusError"));
	}

	controls.title_label->set_text(state.title);

	if (state.status_message.is_empty()) {
		controls.status_label->hide();
	} else {
		controls.status_label->set_text(state.status_message);
		controls.status_label->show();
	}
}

void WelcomeDialog::_update() {
	_update_prerequisite(dotnet_sdk_prerequisite, dotnet_sdk_controls);
	_update_prerequisite(editor_integration_prerequisite, editor_integration_controls);

	external_editor_checkbox_icon->set_texture(get_editor_theme_icon(external_editor_configured ? "GuiChecked" : "GuiUnchecked"));

	if (!prerequisite_step_complete) {
		bool can_continue = dotnet_sdk_prerequisite.fulfilled && editor_integration_prerequisite.fulfilled;
		get_ok_button()->set_text(TTR("Next"));
		get_ok_button()->set_disabled(!can_continue);
		pre_install_step_vb->show();
		post_install_step_vb->hide();
	} else {
		get_ok_button()->set_text(TTR("Save and enable .NET features"));
		get_ok_button()->set_disabled(false);
		pre_install_step_vb->hide();
		post_install_step_vb->show();
	}
}

void WelcomeDialog::_update_external_editor_settings() {
	ExternalEditorID external_editor_id = EditorSettings::get_singleton()->get("dotnet/editor/external_editor");

	external_editor_configured = external_editor_id != EXTERNAL_EDITOR_NONE;

	bool selected_custom_editor = external_editor_id == EXTERNAL_EDITOR_CUSTOM_EDITOR;

	QuickOptions *quick_options = static_cast<QuickOptions *>(external_editor_inspector->get_edited_object());
	if (selected_custom_editor) {
		quick_options->set_properties({
				"dotnet/editor/external_editor",
				"dotnet/editor/custom_exec_path",
				"dotnet/editor/custom_exec_path_args",
		});
	} else {
		quick_options->set_properties({
				"dotnet/editor/external_editor",
		});
	}
	callable_mp(external_editor_inspector, &EditorInspector::update_tree).call_deferred();

	_update();
}

void WelcomeDialog::_external_editor_settings_changed(const String &p_name) {
	const String full_name = "dotnet/editor/" + p_name;
	if (full_name == "dotnet/editor/external_editor") {
		_update_external_editor_settings();
	}
}

static Thread thread;
void WelcomeDialog::_start_download_editor_integration() {
	editor_integration_download_button->hide();
	editor_integration_progress_bar->show();

	editor_integration_prerequisite.fulfilled = false;
	editor_integration_prerequisite.title = TTR("C# editor integration not found");
	editor_integration_prerequisite.status_message = TTR("Downloading C# editor integration...");
	_update();

	thread.start(&WelcomeDialog::_thread_function_download_editor_integration, this);
}

void WelcomeDialog::_thread_function_download_editor_integration(void *p_user_data) {
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	bool success = DotNetModule::get_singleton()->try_restore_editor_packages(editor_assemblies_dir);

	const String message = success ? String() : TTR("Failed to download C# editor integration. Please check your internet connection and try again.");

	// Call this method deferred to ensure it's executed in the main thread.
	WelcomeDialog *wd = static_cast<WelcomeDialog *>(p_user_data);
	callable_mp(wd, &WelcomeDialog::_finish_download_editor_integration).bind(message).call_deferred();
}

void WelcomeDialog::_finish_download_editor_integration(const String &p_error_message) {
	thread.wait_to_finish();

	bool success = p_error_message.is_empty();

	editor_integration_progress_bar->hide();

	editor_integration_prerequisite.fulfilled = success;
	if (success) {
		editor_integration_prerequisite.title = TTR("C# editor integration ready");
		editor_integration_prerequisite.status_message = "";
		editor_integration_download_button->hide();
	} else {
		editor_integration_prerequisite.title = TTR("C# editor integration not found");
		editor_integration_prerequisite.status_message = TTR("Failed to download C# editor integration. Please check your internet connection and try again.");
		editor_integration_download_button->set_text(TTR("Retry download"));
		editor_integration_download_button->show();
	}

	_update();
}

WelcomeDialog::ButtonContainer *WelcomeDialog::_create_button_base(Container *p_container) {
	int margin = 8 * EDSCALE;
	MarginContainer *mc = memnew(MarginContainer);
	mc->add_theme_constant_override("margin_right", margin);
	mc->add_theme_constant_override("margin_top", margin);
	mc->add_theme_constant_override("margin_left", margin);
	mc->add_theme_constant_override("margin_bottom", margin);
	mc->add_child(p_container);
	return memnew(ButtonContainer(mc));
}

Button *WelcomeDialog::_create_editor_suggestion_button(const String &p_editor_name, const String &p_url, WelcomeDialog::ExternalEditorID p_external_editor_id) {
	VBoxContainer *vb = memnew(VBoxContainer);
	WelcomeDialog::ButtonContainer *button = _create_button_base(vb);
	button->set_custom_minimum_size(Size2(128 * EDSCALE, 0));
	button->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
	button->connect(SceneStringName(pressed), callable_mp(this, &WelcomeDialog::_select_editor_suggestion).bind(p_url, p_external_editor_id));

	Label *label = memnew(Label(p_editor_name));
	label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	vb->add_child(label);

	return button;
}

Button *WelcomeDialog::_create_documentation_button(const String &p_title, const String &p_description, const String &p_url) {
	GridContainer *gc = memnew(GridContainer);
	gc->set_columns(2);
	WelcomeDialog::ButtonContainer *button = _create_button_base(gc);
	button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	button->connect(SceneStringName(pressed), callable_mp_static(&WelcomeDialog::_open_url).bind(p_url));

	TextureRect *doc_icon = memnew(TextureRect);
	doc_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	doc_icons.push_back(doc_icon);
	gc->add_child(doc_icon);

	Label *title_label = memnew(Label(TTR(p_title)));
	title_label->set_theme_type_variation("HeaderMedium");
	gc->add_child(title_label);

	gc->add_child(memnew(Control)); // Spacer.

	Label *description_label = memnew(Label(TTR(p_description)));
	description_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	description_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	description_label->set_custom_minimum_size(Size2(128 * EDSCALE, 1));
	gc->add_child(description_label);

	return button;
}

void WelcomeDialog::_select_editor_suggestion(const String &p_url, WelcomeDialog::ExternalEditorID p_external_editor_id) {
	OS::get_singleton()->shell_open(p_url);
	EditorSettings::get_singleton()->set("dotnet/editor/external_editor", p_external_editor_id);
	_update_external_editor_settings();
}

void WelcomeDialog::_open_url(const String &p_url) {
	OS::get_singleton()->shell_open(p_url);
}

void WelcomeDialog::ok_pressed() {
	// Before the prerequisite step is complete, this is the "Next" button.
	if (!prerequisite_step_complete) {
		if (dotnet_sdk_prerequisite.fulfilled && editor_integration_prerequisite.fulfilled) {
			prerequisite_step_complete = true;
			_update();
		}
		return;
	}

	EditorSettings::get_singleton()->notify_changes();
	EditorSettings::get_singleton()->save();

	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	// Avoid initializing again if it's already initialized or in the process of initializing.
	DotNetModuleState::InitState init_state = module->get_state()->get_initialization_state();
	bool is_initialized = init_state == DotNetModuleState::InitState::INITIALIZED || init_state == DotNetModuleState::InitState::INITIALIZING;
	if (!is_initialized) {
		DotNetModule::get_singleton()->initialize();
	}

	hide();
}

void WelcomeDialog::_pre_popup() {
	prerequisite_step_complete = false;

	// When the .NET module has already been initialized, we can skip the prerequisite step.
	DotNetModule *module = DotNetModule::get_singleton();
	if (module != nullptr) {
		prerequisite_step_complete = module->get_state()->get_initialization_state() == DotNetModuleState::InitState::INITIALIZED;
	}

	reset_size();
}

void WelcomeDialog::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_READY:
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible()) {
				// These may have changed since the last time the dialog was opened.
				_check_dotnet_sdk();
				_check_editor_integration();
				_update_external_editor_settings();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update();

			for (uint32_t i = 0; i < doc_icons.size(); i++) {
				const Ref<Texture2D> doc_icon_texture = get_editor_theme_icon("Help");
				doc_icons[i]->set_texture(doc_icon_texture);
			}
		} break;
	}
}

WelcomeDialog::WelcomeDialog() {
	singleton = this;

	set_min_size(Size2(600 * EDSCALE, 400 * EDSCALE));
	set_title(TTR("Welcome to Godot .NET"));
	set_hide_on_ok(false);
	get_ok_button()->set_disabled(true);
	get_ok_button()->set_text(TTR("Save and enable .NET features"));

	VBoxContainer *vb = memnew(VBoxContainer);
	add_child(vb);

	pre_install_step_vb = memnew(VBoxContainer);
	vb->add_child(pre_install_step_vb);

	post_install_step_vb = memnew(VBoxContainer);
	post_install_step_vb->hide();
	vb->add_child(post_install_step_vb);

	// Pre-install header.
	{
		PanelContainer *header = memnew(PanelContainer);
		pre_install_step_vb->add_child(header);

		VBoxContainer *header_vb = memnew(VBoxContainer);
		header->add_child(header_vb);

		Label *description_label = memnew(Label(TTR("Enable .NET features to use C# in Godot.\nLet's get you set up with the few requirements needed to get started!")));
		description_label->set_theme_type_variation("HeaderLarge");
		description_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		description_label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		header_vb->add_child(description_label);
	}

	// Post-install header.
	{
		PanelContainer *header = memnew(PanelContainer);
		post_install_step_vb->add_child(header);

		VBoxContainer *header_vb = memnew(VBoxContainer);
		header->add_child(header_vb);

		Label *description_label = memnew(Label(TTR("One last step to finish setting up!\nTake a quick look at these settings and resources to get the most out of C# in Godot.")));
		description_label->set_theme_type_variation("HeaderLarge");
		description_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		description_label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		header_vb->add_child(description_label);
	}

	// .NET SDK section.
	{
		PanelContainer *dotnet_sdk_panel = memnew(PanelContainer);
		dotnet_sdk_panel->set_theme_type_variation(SNAME("DotNetWelcomeDialogStep"));
		pre_install_step_vb->add_child(dotnet_sdk_panel);

		GridContainer *dotnet_sdk_gc = memnew(GridContainer);
		dotnet_sdk_gc->set_columns(2);
		dotnet_sdk_panel->add_child(dotnet_sdk_gc);

		dotnet_sdk_controls.status_icon = memnew(TextureRect);
		dotnet_sdk_controls.status_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		dotnet_sdk_gc->add_child(dotnet_sdk_controls.status_icon);

		dotnet_sdk_controls.title_label = memnew(Label);
		dotnet_sdk_controls.title_label->set_theme_type_variation("HeaderMedium");
		dotnet_sdk_gc->add_child(dotnet_sdk_controls.title_label);

		dotnet_sdk_gc->add_child(memnew(Control)); // Spacer.

		VBoxContainer *dotnet_sdk_content_vb = memnew(VBoxContainer);
		dotnet_sdk_content_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		dotnet_sdk_gc->add_child(dotnet_sdk_content_vb);

		dotnet_sdk_controls.status_label = memnew(Label);
		dotnet_sdk_controls.status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		dotnet_sdk_controls.status_label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		dotnet_sdk_content_vb->add_child(dotnet_sdk_controls.status_label);

		HBoxContainer *dotnet_sdk_button_container = memnew(HBoxContainer);
		dotnet_sdk_button_container->set_alignment(HBoxContainer::ALIGNMENT_END);
		dotnet_sdk_content_vb->add_child(dotnet_sdk_button_container);

		Button *check_dotnet_sdk_button = memnew(Button);
		check_dotnet_sdk_button->set_text(TTR("Check again"));
		check_dotnet_sdk_button->connect(SceneStringName(pressed), callable_mp(this, &WelcomeDialog::_check_dotnet_sdk));
		dotnet_sdk_button_container->add_child(check_dotnet_sdk_button);

		download_dotnet_sdk_button = memnew(Button);
		download_dotnet_sdk_button->set_text(TTR("Download .NET SDK"));
		download_dotnet_sdk_button->connect(SceneStringName(pressed), callable_mp_static(&WelcomeDialog::_open_url).bind("https://get.dot.net/"));
		dotnet_sdk_button_container->add_child(download_dotnet_sdk_button);
	}

	// Godot Editor Integration section.
	{
		PanelContainer *editor_integration_panel = memnew(PanelContainer);
		editor_integration_panel->set_theme_type_variation(SNAME("DotNetWelcomeDialogStep"));
		pre_install_step_vb->add_child(editor_integration_panel);

		GridContainer *editor_integration_gc = memnew(GridContainer);
		editor_integration_gc->set_columns(2);
		editor_integration_panel->add_child(editor_integration_gc);

		editor_integration_controls.status_icon = memnew(TextureRect);
		editor_integration_controls.status_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		editor_integration_gc->add_child(editor_integration_controls.status_icon);

		editor_integration_controls.title_label = memnew(Label(TTR("C# editor integration not available")));
		editor_integration_controls.title_label->set_theme_type_variation("HeaderMedium");
		editor_integration_gc->add_child(editor_integration_controls.title_label);

		editor_integration_gc->add_child(memnew(Control)); // Spacer.

		VBoxContainer *editor_integration_content_vb = memnew(VBoxContainer);
		editor_integration_content_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		editor_integration_gc->add_child(editor_integration_content_vb);

		editor_integration_controls.status_label = memnew(Label);
		editor_integration_controls.status_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		editor_integration_controls.status_label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		editor_integration_content_vb->add_child(editor_integration_controls.status_label);

		HBoxContainer *editor_integration_button_container = memnew(HBoxContainer);
		editor_integration_button_container->set_alignment(HBoxContainer::ALIGNMENT_END);
		editor_integration_content_vb->add_child(editor_integration_button_container);

		editor_integration_download_button = memnew(Button);
		editor_integration_download_button->connect(SceneStringName(pressed), callable_mp(this, &WelcomeDialog::_start_download_editor_integration));
		editor_integration_button_container->add_child(editor_integration_download_button);

		editor_integration_progress_bar = memnew(ProgressBar);
		editor_integration_progress_bar->set_indeterminate(true);
		editor_integration_progress_bar->hide();
		editor_integration_content_vb->add_child(editor_integration_progress_bar);
	}

	// External Editor section.
	{
		PanelContainer *external_editor_panel = memnew(PanelContainer);
		external_editor_panel->set_theme_type_variation(SNAME("DotNetWelcomeDialogStep"));
		post_install_step_vb->add_child(external_editor_panel);

		GridContainer *external_editor_gc = memnew(GridContainer);
		external_editor_gc->set_columns(2);
		external_editor_panel->add_child(external_editor_gc);

		external_editor_checkbox_icon = memnew(TextureRect);
		external_editor_checkbox_icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		external_editor_gc->add_child(external_editor_checkbox_icon);

		Label *title = memnew(Label(TTR("Configure an external editor")));
		title->set_theme_type_variation("HeaderMedium");
		external_editor_gc->add_child(title);

		external_editor_gc->add_child(memnew(Control)); // Spacer.

		VBoxContainer *external_editor_vb = memnew(VBoxContainer);
		external_editor_gc->add_child(external_editor_vb);

		Label *label = memnew(Label(TTR("Install your preferred editor to get support for autocompletion, debugging, and other useful features for C#. Then, configure Godot to use it when opening C# files.")));
		label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		external_editor_vb->add_child(label);

		external_editor_inspector = memnew(EditorInspector);
		external_editor_inspector->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
		external_editor_inspector->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		external_editor_inspector->set_use_doc_hints(true);
		external_editor_inspector->connect(SNAME("property_edited"), callable_mp(this, &WelcomeDialog::_external_editor_settings_changed));
		external_editor_vb->add_child(external_editor_inspector);
		QuickOptions *quick_options = memnew(QuickOptions);
		quick_options->set_section("dotnet/editor");
		external_editor_inspector->set_property_prefix("dotnet/editor/");
		external_editor_inspector->edit(quick_options);

		Label *label2 = memnew(Label(TTR("If you don't have a preferred editor yet, here are some popular options to download:")));
		label2->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		label2->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		external_editor_vb->add_child(label2);

		HBoxContainer *editor_suggestions_hb = memnew(HBoxContainer);
		external_editor_vb->add_child(editor_suggestions_hb);

		PanelContainer *recommended_container = memnew(PanelContainer);
		recommended_container->set_theme_type_variation("RecommendedPanelContainer");
		editor_suggestions_hb->add_child(recommended_container);

		VBoxContainer *recommended_container_vb = memnew(VBoxContainer);
		recommended_container->add_child(recommended_container_vb);

		Button *vscode_button = _create_editor_suggestion_button("Visual Studio Code", "https://code.visualstudio.com/", EXTERNAL_EDITOR_VSCODE);
		recommended_container_vb->add_child(vscode_button);
#ifdef WINDOWS_ENABLED
		Button *vs_button = _create_editor_suggestion_button("Visual Studio", "https://visualstudio.microsoft.com/", EXTERNAL_EDITOR_VISUAL_STUDIO);
		editor_suggestions_hb->add_child(vs_button);
#endif
		Button *rider_button = _create_editor_suggestion_button("JetBrains Rider", "https://www.jetbrains.com/rider/", EXTERNAL_EDITOR_JETBRAINS_RIDER);
		editor_suggestions_hb->add_child(rider_button);

		Label *recommended_label = memnew(Label(TTR("Recommended")));
		recommended_label->set_theme_type_variation("HeaderSmall");
		recommended_label->set_horizontal_alignment(HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
		recommended_container_vb->add_child(recommended_label);

		recommended_container->set_clip_children_mode(CanvasItem::CLIP_CHILDREN_AND_DRAW);
	}

	// Helpful links section.
	{
		PanelContainer *documentation_panel = memnew(PanelContainer);
		documentation_panel->set_theme_type_variation(SNAME("DotNetWelcomeDialogStep"));
		post_install_step_vb->add_child(documentation_panel);

		VBoxContainer *documentation_vb = memnew(VBoxContainer);
		documentation_panel->add_child(documentation_vb);

		Label *label = memnew(Label(TTR("Learn more about C# and how to use it in Godot")));
		label->set_theme_type_variation("HeaderMedium");
		label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		label->set_custom_minimum_size(Size2(600 * EDSCALE, 1));
		documentation_vb->add_child(label);

		HBoxContainer *doc_buttons_hb = memnew(HBoxContainer);
		documentation_vb->add_child(doc_buttons_hb);

		doc_buttons_hb->add_child(_create_documentation_button("New to C#?", "Learn the basics with Microsoft's official C# documentation. Godot uses standard C#, so this is the perfect place to start.", "https://learn.microsoft.com/en-us/dotnet/csharp/"));
		doc_buttons_hb->add_child(_create_documentation_button("New to Godot?", "Read the Godot-specific C# documentation. It covers how to use C# effectively within the engine and highlights any differences from standard C#.", "https://docs.godotengine.org/en/stable/tutorials/scripting/c_sharp"));
		doc_buttons_hb->add_child(_create_documentation_button("Used GDScript before?", "See the C# vs GDScript documentation page to learn about the key differences that could trip you up.", "https://docs.godotengine.org/en/stable/tutorials/scripting/c_sharp/c_sharp_differences.html"));
	}
}

WelcomeDialog::~WelcomeDialog() {
	singleton = nullptr;
}

} // namespace DotNet
#endif
