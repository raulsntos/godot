/**************************************************************************/
/*  welcome_dialog.h                                                      */
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

#ifndef DOTNET_WELCOME_DIALOG_ENABLED
#error Welcome Dialog is disabled as it's a work in progress and should not be used for now.
#endif

#ifdef DOTNET_WELCOME_DIALOG_ENABLED
#include "editor/inspector/editor_inspector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/texture_rect.h"

namespace DotNet {

class WelcomeDialog : public AcceptDialog {
	GDCLASS(WelcomeDialog, AcceptDialog)

	enum ExternalEditorID {
		EXTERNAL_EDITOR_NONE,
		EXTERNAL_EDITOR_VISUAL_STUDIO,
		EXTERNAL_EDITOR_VISUAL_STUDIO_FOR_MAC,
		EXTERNAL_EDITOR_MONODEVELOP,
		EXTERNAL_EDITOR_VSCODE,
		EXTERNAL_EDITOR_JETBRAINS_RIDER,
		EXTERNAL_EDITOR_CUSTOM_EDITOR,
		EXTERNAL_EDITOR_JETBRAINS_FLEET,
	};

	class QuickOptions : public Object {
		GDCLASS(QuickOptions, Object);

	private:
		PackedStringArray properties;
		String section;

	protected:
		bool _set(const StringName &p_name, const Variant &p_value);
		bool _get(const StringName &p_name, Variant &r_ret) const;
		void _get_property_list(List<PropertyInfo> *p_list) const;
		bool _property_can_revert(const StringName &p_name) const;
		bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

	public:
		void set_section(const String &p_section);
		void set_properties(PackedStringArray p_properties);
	};

	class ButtonContainer : public Button {
		GDCLASS(ButtonContainer, Button);

	private:
		Container *container = nullptr;

	protected:
		virtual Size2 get_minimum_size() const override;
		void _notification(int p_notification);

	public:
		Container *get_container() const { return container; }
		ButtonContainer(Container *p_container);
	};

	struct PreRequisiteState {
		bool fulfilled = false;
		String title;
		String status_message;
	};

	struct PreRequisiteControls {
		TextureRect *status_icon = nullptr;
		Label *title_label = nullptr;
		Label *status_label = nullptr;
	};

private:
	static WelcomeDialog *singleton;

public:
	static WelcomeDialog *get_singleton();

private:
	VBoxContainer *pre_install_step_vb = nullptr;
	VBoxContainer *post_install_step_vb = nullptr;
	bool prerequisite_step_complete = false;

	PreRequisiteState dotnet_sdk_prerequisite;
	PreRequisiteControls dotnet_sdk_controls;
	Button *download_dotnet_sdk_button = nullptr;

	PreRequisiteState editor_integration_prerequisite;
	PreRequisiteControls editor_integration_controls;
	Button *editor_integration_download_button = nullptr;
	ProgressBar *editor_integration_progress_bar = nullptr;

	bool external_editor_configured = false;
	TextureRect *external_editor_checkbox_icon = nullptr;
	EditorInspector *external_editor_inspector = nullptr;

	LocalVector<TextureRect *> doc_icons;

	void _check_dotnet_sdk();
	void _check_editor_integration();
	void _update_external_editor_settings();

	void _update_prerequisite(PreRequisiteState state, PreRequisiteControls controls);
	void _update();

	// Download management.
	void _external_editor_settings_changed(const String &p_name);
	void _start_download_editor_integration();
	static void _thread_function_download_editor_integration(void *p_user_data);
	void _finish_download_editor_integration(const String &p_error_message);

	WelcomeDialog::ButtonContainer *_create_button_base(Container *p_container);
	Button *_create_editor_suggestion_button(const String &p_editor_name, const String &p_url, ExternalEditorID p_external_editor_id);
	Button *_create_documentation_button(const String &p_title, const String &p_description, const String &p_url);

	void _select_editor_suggestion(const String &p_url, ExternalEditorID p_external_editor_id);
	static void _open_url(const String &p_url);

protected:
	virtual void ok_pressed() override;
	virtual void _pre_popup() override;
	void _notification(int p_notification);

public:
	WelcomeDialog();
	~WelcomeDialog();
};

} // namespace DotNet

VARIANT_ENUM_CAST(DotNet::WelcomeDialog::ExternalEditorID);
#endif
